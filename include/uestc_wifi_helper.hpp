#pragma once
#include "uestc_wifi.hpp"


UESTC_WIFI_HELPER_NS_BEGIN(config)

struct UESTCWifiConfig {
    std::string username;
    std::string password;
    uint32_t network_operator;
    uint32_t check_interval;
};

UESTCWifiConfig load_from_file(std::string_view file);

UESTC_WIFI_HELPER_NS_END


UESTC_WIFI_HELPER_NS_BEGIN()

class UESTCWifiHelper {
public:
    UESTCWifiHelper(std::string_view config_path);
    UESTCWifiHelper(UESTCWifiHelper&&) = default;
    UESTCWifiHelper(const UESTCWifiHelper&) = delete;
    void run() const;
    void stop() const;
private:
    mutable bool running_ { false };
    config::UESTCWifiConfig config_;
    UESTCWifi uestc_wifi_;
};

UESTC_WIFI_HELPER_NS_END
