#pragma once
#include "uestc_wifi.hpp"
#include "uestc_wifi_config.hpp"


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
