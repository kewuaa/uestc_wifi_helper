#include <signal.h>
#include <filesystem>

#include "spdlog/spdlog.h"

#include "uestc_wifi_helper.hpp"
namespace fs = std::filesystem;
using namespace UESTC_WIFI_HELPER_NS;


UESTCWifiHelper* helper { nullptr };


void exit_helper(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        helper->stop();
    }
}


int main() {
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    fs::path home { std::getenv("HOME") };
    auto config_path = home / "uestc_wifi.toml";
    if (!fs::exists(config_path)) {
        SPDLOG_ERROR("config file `{}` not exists, check `template.toml`", config_path.string());
        exit(0);
    }
    UESTCWifiHelper wifi_helper { config_path.string() };
    helper = &wifi_helper;
    signal(SIGINT, exit_helper);
    signal(SIGTERM, exit_helper);
    helper->run();
    return 0;
}
