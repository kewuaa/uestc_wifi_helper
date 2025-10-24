#include <signal.h>

#include "spdlog/spdlog.h"

#include "uestc_wifi_helper.hpp"
using namespace UESTC_WIFI_HELPER_NS;


UESTCWifiHelper helper("config.toml");


void exit_helper(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        helper.stop();
    }
}


int main() {
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    signal(SIGINT, exit_helper);
    signal(SIGTERM, exit_helper);
    helper.run();
    return 0;
}
