#include <filesystem>
#include <string_view>

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include "uestc_wifi_helper.hpp"


UESTC_WIFI_HELPER_NS_BEGIN()

UESTCWifiHelper::UESTCWifiHelper(std::string_view config_path):
    config_(config::load_from_file(config_path)),
    uestc_wifi_(
        config_.username,
        config_.password,
        *magic_enum::enum_cast<UESTCWifi::NetworkOperator>(config_.network_operator)
    ) {
    SPDLOG_INFO(
        "\n"
        "================================\n"
        "   config file      : {}\n"
        "   username         : {}\n"
        "   password         : {}\n"
        "   operator         : {}\n"
        "   check_interval(s): {}\n"
        "================================"
        ,
        std::filesystem::absolute(config_path).string(),
        config_.username,
        config_.password,
        magic_enum::enum_name(UESTCWifi::NetworkOperator(config_.network_operator)),
        config_.check_interval
    );
}

void UESTCWifiHelper::stop() const {
    running_ = false;
    SPDLOG_INFO("uestc wifi helper stopped");
}

UESTC_WIFI_HELPER_NS_END
