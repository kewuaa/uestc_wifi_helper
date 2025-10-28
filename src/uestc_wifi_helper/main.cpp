#include <CLI/CLI.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Validators.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "utils.hpp"
#include "uestc_wifi_helper.hpp"
using namespace UESTC_WIFI_HELPER_NS;


int main(int argc, char** argv) {
    CLI::App app;
    argv = app.ensure_utf8(argv);

    std::string config_path;
    auto home = utils::home_path();

    app.add_option_function<std::string>(
        "--log,-l",
        [](const std::string& path) {
            auto logger = spdlog::basic_logger_mt("uestc_wifi_helper", path);
            spdlog::set_default_logger(logger);
        },
        "logging file path"
    )
        ->run_callback_for_default()
        ->default_val((home/"uestc_wifi.log").string())
        ->capture_default_str();

    app.add_flag_callback(
        "--debug,-d",
        []() {
            spdlog::set_level(spdlog::level::debug);
        },
        "enable debug mode"
    );

    app.add_option_function<std::string>(
        "--config,-c",
        UESTCWifiHelper::init,
        "config file path"
    )
        ->check(CLI::ExistingFile)
        ->run_callback_for_default()
        ->default_val((home/"uestc_wifi.toml").string())
        ->capture_default_str();

    CLI11_PARSE(app, argc, argv);

    UESTCWifiHelper::set_signal_handle();
    UESTCWifiHelper::init().run();
}
