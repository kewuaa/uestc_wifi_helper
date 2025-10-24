#pragma once
#include <string>
#include <cstdint>

#include "uestc_wifi_helper_ns.hpp"


UESTC_WIFI_HELPER_NS_BEGIN(config)

struct UESTCWifiConfig {
    std::string username;
    std::string password;
    uint32_t network_operator;
    uint32_t check_interval;
};

UESTCWifiConfig load_from_file(std::string_view file);

UESTC_WIFI_HELPER_NS_END
