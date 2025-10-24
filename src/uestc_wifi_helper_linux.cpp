#include <chrono>
#include <sys/poll.h>

#include <spdlog/spdlog.h>
#include <sdbus-c++/sdbus-c++.h>
#include <magic_enum/magic_enum.hpp>

#include "uestc_wifi_helper.hpp"


namespace {

constexpr const char* NOTIFY_SERVICE = "org.freedesktop.Notifications";
constexpr const char* NOTIFY_PATH = "/org/freedesktop/Notifications";
constexpr const char* NOTIFY_INTERFACE = "org.freedesktop.Notifications";
constexpr const char* NOTIFY_METHOD = "Notify";

constexpr const char* NM_SERVICE = "org.freedesktop.NetworkManager";
constexpr const char* NM_PATH = "/org/freedesktop/NetworkManager";
constexpr const char* NM_INTERFACE = "org.freedesktop.NetworkManager";
constexpr const char* NM_SIGNAL = "StateChanged";

// NetworkManager 的状态 (NMState)
// 参考: https://people.freedesktop.org/~lkundrak/nm-docs/nm-dbus-types.html#NMState
enum class NMState {
    UNKNOWN = 0,
    ASLEEP = 10,
    DISCONNECTED = 20,
    DISCONNECTING = 30,
    CONNECTING = 40,
    CONNECTED_LOCAL = 50,
    CONNECTED_SITE = 60,
    CONNECTED_GLOBAL = 70
};

}


UESTC_WIFI_HELPER_NS_BEGIN()

void UESTCWifiHelper::run() const {
    using namespace std::chrono;

    pollfd fd;
    fd.fd = -1;
    fd.events = POLLIN;

    auto conn = sdbus::createBusConnection();
    auto sys_conn = sdbus::createSystemBusConnection();
    auto notify_proxy = sdbus::createProxy(*conn.get(), sdbus::ServiceName(NOTIFY_SERVICE), sdbus::ObjectPath(NOTIFY_PATH));
    auto nm_proxy = sdbus::createProxy(*sys_conn.get(), sdbus::ServiceName(NM_SERVICE), sdbus::ObjectPath(NM_PATH));
    auto state = nm_proxy->getProperty("State").onInterface(NM_INTERFACE).get<uint32_t>();
    bool connected_global = state == (uint32_t)NMState::CONNECTED_SITE || state == (uint32_t)NMState::CONNECTED_GLOBAL;
    nm_proxy->uponSignal(NM_SIGNAL).onInterface(NM_INTERFACE).call([&connected_global](uint32_t state) {
        SPDLOG_DEBUG(
            "NetworkManager state changed, new state: ({})",
            magic_enum::enum_name(*magic_enum::enum_cast<NMState>(state))
        );
        switch (NMState(state)) {
            case NMState::UNKNOWN:
            case NMState::ASLEEP:
            case NMState::DISCONNECTED:
            case NMState::CONNECTED_LOCAL: {
                connected_global = false;
                break;
            }
            case NMState::CONNECTED_SITE:
            case NMState::CONNECTED_GLOBAL: {
                connected_global = true;
                break;
            }
            default: {
                break;
            }
        }
    });
    auto notify = [&notify_proxy](std::string_view text) {
        notify_proxy->callMethod(NOTIFY_METHOD)
            .onInterface(NOTIFY_INTERFACE)
            .withArguments(
                "uestc_wifi_helper",
                0u,
                "",
                "UESTC Wifi 助手",
                text,
                std::vector<std::string>(),
                std::unordered_map<std::string, sdbus::Variant>(),
                -1
            );
    };

    running_ = true;
    milliseconds check_interval = seconds(config_.check_interval);
    while (running_) {
        if (connected_global) {
            if (auto state = uestc_wifi_.check_online(); state.has_value()) {
                auto [online, ip] = *state;
                if (!online) {
                    notify("检测到用户已下线");
                    auto res = uestc_wifi_.login(ip);
                    if (res) {
                        SPDLOG_DEBUG("login successfully");
                        notify("登陆成功，用户已重新上线");
                    } else {
                        SPDLOG_ERROR("login failed: {}", UESTCWifi::translate_error(res.error()));
                        switch (res.error()) {
                            case UESTCWifi::Error::IncorrectUsernameOrPassword: {
                                notify("用户名或密码错误，请检查配置文件");
                                stop();
                                break;
                            }
                            case UESTCWifi::Error::DeviceWithinScope: {
                                notify("设备不再范围内");
                                stop();
                                break;
                            }
                            case UESTCWifi::Error::AuthRequestsFrequently: {
                                notify("认证请求过于频繁，建议增加检查间隔时间");
                                stop();
                                break;
                            }
                            default: {
                                break;
                            }
                        }
                    }
                } else {
                    SPDLOG_DEBUG("already online: {}", ip);
                }
            } else {
                SPDLOG_ERROR("check online failed: {}", UESTCWifi::translate_error(state.error()));
            }
        }

        if (!running_) {
            std::this_thread::sleep_for(seconds(1));
            break;
        }

        auto data = sys_conn->getEventLoopPollData();
        auto timeout = check_interval.count();
        if (data.timeout.count() > 0) {
            timeout = std::min(timeout, data.timeout.count());
        }
        if (!connected_global) {
            timeout = -1;
        }
        fd.fd = data.fd;
        auto ret = poll(&fd, 1, timeout);
        if (ret == -1 && errno != EINTR) {
            throw std::runtime_error(std::format("poll error: {}", std::strerror(errno)));
        }
        // timeout
        if (ret == 0) {
            SPDLOG_DEBUG("poll timeout: {}", timeout);
            continue;
        }
        sys_conn->processPendingEvent();
    }
    notify("程序已退出");
}

UESTC_WIFI_HELPER_NS_END
