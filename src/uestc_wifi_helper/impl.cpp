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

UESTCWifiHelper& UESTCWifiHelper::init(std::string_view config_path) {
    static UESTCWifiHelper singleton(config_path);
    return singleton;
}

void UESTCWifiHelper::check_once(const std::function<void(std::string_view)>& notify) const {
    std::string msg;
    if (auto state = uestc_wifi_.check_online(); state.has_value()) {
        auto [online, ip] = *state;
        if (online) {
            SPDLOG_DEBUG("already online: {}", ip);
            return;
        } else {
            if (notify) notify("检测到用户已下线");
            auto res = uestc_wifi_.login(ip);
            if (res) {
                SPDLOG_DEBUG("login successfully");
                msg = "登陆成功，用户已重新上线";
            } else {
                SPDLOG_ERROR("login failed: {}", UESTCWifi::translate_error(res.error()));
                switch (res.error()) {
                    case UESTCWifi::Error::IncorrectUsernameOrPassword: {
                        msg = "用户名或密码错误，请检查配置文件";
                        stop();
                        break;
                    }
                    case UESTCWifi::Error::DeviceWithinScope: {
                        msg = "设备不在范围内";
                        stop();
                        break;
                    }
                    case UESTCWifi::Error::AuthRequestsFrequently: {
                        msg = "认证请求过于频繁，建议增加检查间隔时间";
                        stop();
                        break;
                    }
                    case UESTCWifi::Error::HttplibError: {
                        msg = std::format("unexpected error: {}", UESTCWifi::translate_error(res.error()));
                        break;
                    }
                    case UESTCWifi::Error::NetworkConnectionTimeout: return;
                }
            }
        }
    } else {
        SPDLOG_ERROR("check online failed: {}", UESTCWifi::translate_error(state.error()));
        msg = std::format("unexpected error: {}", UESTCWifi::translate_error(state.error()));
    }
    if (notify) notify(msg);
}

UESTC_WIFI_HELPER_NS_END
