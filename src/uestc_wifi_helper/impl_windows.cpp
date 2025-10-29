#include <windows.h>

#include <spdlog/spdlog.h>
#include <wil/winrt.h>
#include <wintoastlib.h>

#include "nlm.hpp"
#include "uestc_wifi_helper.hpp"


namespace {
using namespace UESTC_WIFI_HELPER_NS;
using namespace WinToastLib;

BOOL WINAPI exit_helper(DWORD sig) {
    switch (sig) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            UESTCWifiHelper::init().stop();
            return TRUE;
        default: return FALSE;
    }
}

std::wstring to_wstring(std::string_view str) {
    if (str.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring wide_str(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), wide_str.data(), size_needed);
    return wide_str;
}

std::string from_wstring(std::wstring_view wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), size_needed, nullptr, nullptr);
    return str;
}

class WinToastHandlerExample : public IWinToastHandler {
public:
    WinToastHandlerExample() {}

    void toastActivated() const override {}
    void toastActivated(int actionIndex) const override {}
    void toastActivated(std::wstring response) const override {}
    void toastDismissed(WinToastDismissalReason state) const override {}
    void toastFailed() const override {}
};

}


UESTC_WIFI_HELPER_NS_BEGIN()

void UESTCWifiHelper::run() const {
    using namespace nlm;
    using namespace std::chrono;
    using namespace WinToastLib;

    auto coinit = wil::CoInitializeEx_failfast(COINIT_MULTITHREADED);

    NLM nlm;
    bool local_connected = nlm.check_connectivity() != NLM::Connectivity::Disconnected;
    auto token = nlm.register_callback([&](NLM_CONNECTIVITY connectivity) {
        auto state = nlm.check_connectivity(connectivity);
        SPDLOG_DEBUG("Network List Manager state changed, new state: ({})", magic_enum::enum_name(state));
        local_connected = state != NLM::Connectivity::Disconnected;
    });

    const std::wstring wtitle = to_wstring(UESTCWifiHelper::TITLE);
    std::function<void(std::string_view)> notify{ nullptr };
    if (!WinToast::isCompatible()) {
        SPDLOG_WARN("WinToast is not compatible with this system.");
    } else {
        auto instance = WinToast::instance();
        instance->setAppName(L"uestc_wifi_helper");
        const auto aumi = WinToast::configureAUMI(L"kewuaa", L"uestc_wifi_helper", L"notification", L"20230901");
        instance->setAppUserModelId(aumi);

        WinToast::WinToastError error;
        const auto succeeded = instance->initialize(&error);
        if (!succeeded) {
            SPDLOG_ERROR(
                "WinToast initialization failed: {}",
                from_wstring(WinToast::strerror(error))
            );
        } else {
            notify = [error = std::move(error)](std::string_view msg) mutable {
                auto handler = new WinToastHandlerExample();
                WinToastTemplate toast(WinToastTemplate::Text02);
                toast.setFirstLine(to_wstring(UESTCWifiHelper::TITLE));
                toast.setSecondLine(to_wstring(msg));
                toast.setDuration(WinToastTemplate::Short);
                auto id = WinToast::instance()->showToast(toast, handler, &error);
                if (id < 0) {
                    SPDLOG_ERROR(
                        "Failed to show toast notification: {}",
                        from_wstring(WinToast::strerror(error))
                    );
                }
            };
        }
    }


    running_ = true;
    while (running_) {
        if (local_connected) {
            check_once(notify);
        }
        if (!running_) {
            break;
        }
        SPDLOG_DEBUG("sleep {} seconds", config_.check_interval);
        std::this_thread::sleep_for(seconds(config_.check_interval));
    }
}

void UESTCWifiHelper::set_signal_handle() {
    SetConsoleCtrlHandler(exit_helper, TRUE);
}

UESTC_WIFI_HELPER_NS_END
