#include <CLI/App.hpp>
#include <spdlog/spdlog.h>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <magic_enum/magic_enum.hpp>

#include "./utils.hpp"
#include "uestc_wifi.hpp"
using namespace std::ranges;
using namespace UESTC_WIFI_HELPER_NS;


int main(int argc, char** argv) {
    CLI::App app;
    argv = app.ensure_utf8(argv);

    std::string username;
    std::string password;
    UESTCWifi::NetworkOperator op;

    app.add_option("--username,-n", username, "login username")->required();
    app.add_option("--password,-p", password, "login password")->required();
    app.add_option_function<std::string>(
        "--operator,-o",
        [&op](const std::string& str) {
            op = *magic_enum::enum_cast<UESTCWifi::NetworkOperator>(str);
        },
        std::format("network operator({})", utils::join_enum<UESTCWifi::NetworkOperator>())
    )
        ->check([](const std::string& str) {
            if (!magic_enum::enum_contains<UESTCWifi::NetworkOperator>(str)) {
                return "Invalid network operator, -h for help";
            }
            return "";
        })
        ->default_val("CTCC")
        ->capture_default_str();
    app.add_flag_callback("--dry-run", [&]() {
        SPDLOG_INFO(
            "\n"
            "======parse options below:======\n"
            "   username: {}\n"
            "   password: {}\n"
            "   operator: {}\n"
            "================================"
            ,
            username,
            password,
            magic_enum::enum_name(op)
        );
        exit(0);
    });
    CLI11_PARSE(app, argc, argv);

    UESTCWifi wifi(username, password, op);
    if (auto state = wifi.check_online(); state.has_value()) {
        auto [online, ip] = *state;
        if (!online ) {
            auto _ = wifi.login(ip).or_else([](auto err) -> UESTCWifi::Result<void> {
                SPDLOG_ERROR("login failed: {}", UESTCWifi::translate_error(err));
                return {};
            });
        } else {
            SPDLOG_INFO("already online: {}", ip);
        }
    } else {
        SPDLOG_ERROR("check online failed: {}", UESTCWifi::translate_error(state.error()));
    }
}

