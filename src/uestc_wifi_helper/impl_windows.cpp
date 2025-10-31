#include <queue>
#include <windows.h>

#include <spdlog/spdlog.h>
#include <wil/winrt.h>

#include "tray.h"
#include "nlm.hpp"
#include "uestc_wifi_helper.hpp"
// #ifndef _DEBUG
// #pragma comment(linker, "/subsystem:\"Windows\" /entry:\"mainCRTStartup\"")
// #endif


namespace {
using namespace UESTC_WIFI_HELPER_NS;

std::wstring to_wstring(std::string_view str) {
    if (str.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring wide_str(size_needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), wide_str.data(), size_needed);
    return wide_str;
}

void quit_cb(struct tray_menu_item* item) {
    UESTCWifiHelper::init().stop();
    tray_exit();
}

#ifdef _DEBUG
BOOL WINAPI exit_helper(DWORD sig) {
    auto wtitle = to_wstring(UESTCWifiHelper::TITLE);
    switch (sig) {
        case CTRL_C_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            UESTCWifiHelper::init().stop();
            tray_message(wtitle.c_str(), L"正在退出...");
            tray_exit();
            return TRUE;
        default: return FALSE;
    }
}
#endif

}


UESTC_WIFI_HELPER_NS_BEGIN()

void UESTCWifiHelper::run() const {
    using namespace nlm;
    using namespace std::chrono;

#ifdef _DEBUG
    // hide console window
    auto hwnd = GetConsoleWindow();
    if (hwnd) ShowWindow(hwnd, SW_HIDE);
#endif

    auto coinit = wil::CoInitializeEx_failfast(COINIT_MULTITHREADED);

    NLM nlm;
    bool local_connected = nlm.check_connectivity() != NLM::Connectivity::Disconnected;
    auto token = nlm.register_callback([&](NLM_CONNECTIVITY connectivity) {
        auto state = nlm.check_connectivity(connectivity);
        SPDLOG_DEBUG("Network List Manager state changed, new state: ({})", magic_enum::enum_name(state));
        local_connected = state != NLM::Connectivity::Disconnected;
    });

    const std::wstring wtitle = to_wstring(UESTCWifiHelper::TITLE);
    tray_menu_item menu_items[] = {
        { .text = "quit" , .cb = quit_cb },
        { .text = NULL }
    };
    struct tray tray = {
        .icon_filepath = __argv[0],
        .tooltip = wtitle.c_str(),
        .cb = nullptr,
        .menu = menu_items,
    };
    if (tray_init(&tray) < 0) {
        SPDLOG_ERROR("Failed to initialize system tray icon.");
        return;
    }

    std::queue<std::wstring> msg_qq;
    std::function<void(std::string_view)> notify = [&msg_qq](std::string_view msg) {
        msg_qq.push(to_wstring(msg));
    };

    running_ = true;
    int t = 0;
    int T = config_.check_interval * 10;
    while (tray_loop(0) == 0) {
        if (t == 0 && local_connected) {
            check_once(notify);
        }
        if (!msg_qq.empty()) {
            tray_message(wtitle.c_str(), msg_qq.front().c_str());
            msg_qq.pop();
        }
        t = (t + 1) % T;
        std::this_thread::sleep_for(milliseconds(100));
    }
}

void set_signal_handle() {
#ifdef _DEBUG
    SetConsoleCtrlHandler(exit_helper, TRUE);
#endif
}

UESTC_WIFI_HELPER_NS_END
