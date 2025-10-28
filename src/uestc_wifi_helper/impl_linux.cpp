#include <chrono>
#include <signal.h>
#include <sys/poll.h>

#include <spdlog/spdlog.h>
#include <sdbus-c++/sdbus-c++.h>
#include <magic_enum/magic_enum.hpp>

#include "uestc_wifi_helper.hpp"


namespace {
using namespace UESTC_WIFI_HELPER_NS;

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

void exit_helper(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            UESTCWifiHelper::init().stop();
    }
}

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
    bool local_connected = state == (uint32_t)NMState::CONNECTED_SITE || state == (uint32_t)NMState::CONNECTED_GLOBAL;
    nm_proxy->uponSignal(NM_SIGNAL).onInterface(NM_INTERFACE).call([&local_connected](uint32_t state) {
        SPDLOG_DEBUG(
            "NetworkManager state changed, new state: ({})",
            magic_enum::enum_name(*magic_enum::enum_cast<NMState>(state))
        );
        switch (NMState(state)) {
            case NMState::UNKNOWN:
            case NMState::ASLEEP:
            case NMState::DISCONNECTED:
            case NMState::CONNECTED_LOCAL: {
                local_connected = false;
                break;
            }
            case NMState::CONNECTED_SITE:
            case NMState::CONNECTED_GLOBAL: {
                local_connected = true;
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
                TITLE,
                text,
                std::vector<std::string>(),
                std::unordered_map<std::string, sdbus::Variant>(),
                -1
            );
    };

    running_ = true;
    milliseconds check_interval = seconds(config_.check_interval);
    while (running_) {
        if (local_connected) {
            check_once(notify);
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
        if (!local_connected) {
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

void UESTCWifiHelper::set_signal_handle() {
    signal(SIGINT, exit_helper);
    signal(SIGTERM, exit_helper);
}

UESTC_WIFI_HELPER_NS_END
