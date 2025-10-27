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
    static constexpr const char* TITLE { "UESTC Wifi 助手" };

    UESTCWifiHelper(UESTCWifiHelper&&) = delete;
    UESTCWifiHelper(const UESTCWifiHelper&) = delete;
    void run() const;
    void stop() const;
    static UESTCWifiHelper& init(std::string_view config_path = "");
    static void set_signal_handle();
    void check_once(const std::function<void(std::string_view)>& notify) const;

    inline bool is_running() const {
        return running_;
    }
private:
    mutable bool running_ { false };
    config::UESTCWifiConfig config_;
    UESTCWifi uestc_wifi_;

    UESTCWifiHelper(std::string_view config_path);
};

UESTC_WIFI_HELPER_NS_END
