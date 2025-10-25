#include <format>

#include <toml++/toml.hpp>

#include "uestc_wifi_helper.hpp"


UESTC_WIFI_HELPER_NS_BEGIN(config)

UESTCWifiConfig load_from_file(std::string_view file) {
    toml::table table = toml::parse_file(file);
    auto username = table["username"].value<std::string>();
    if (!username) {
        throw std::runtime_error(std::format("`username` not found in `{}`", file));
    }
    auto password = table["password"].value<std::string>();
    if (!password) {
        throw std::runtime_error(std::format("`password` not found in `{}`", file));
    }
    auto network_operator = table["network_operator"].value<uint32_t>().value_or(0);
    auto check_interval = table["check_interval"].value<uint32_t>().value_or(1);
    return {
        *username,
        *password,
        network_operator,
        check_interval,
    };
}

UESTC_WIFI_HELPER_NS_END
