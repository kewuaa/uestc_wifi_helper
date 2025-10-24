#include <signal.h>
#include <filesystem>

#include "CLI/CLI.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"
#include "CLI/Validators.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "uestc_wifi_helper.hpp"
namespace fs = std::filesystem;
using namespace UESTC_WIFI_HELPER_NS;


UESTCWifiHelper* helper { nullptr };


void exit_helper(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        helper->stop();
    }
}


int main(int argc, char** argv) {
    CLI::App app;
    argv = app.ensure_utf8(argv);

    std::string config_path;
    fs::path home { std::getenv("HOME") };

    app.add_option_function<std::string>(
        "--config,-c",
        [](const std::string& path) {
            static UESTCWifiHelper wifi_helper { path };
            helper = &wifi_helper;
        },
        "config file path"
    )
        ->check(CLI::ExistingFile)
        ->default_val((home/"uestc_wifi.toml").string())
        ->run_callback_for_default()
        ->capture_default_str();

    app.add_option_function<std::string>(
        "--log,-l",
        [](const std::string& path) {
            auto logger = spdlog::basic_logger_mt("uestc_wifi_helper", path);
            spdlog::set_default_logger(logger);
        },
        "logging file path"
    )
        ->default_val((home/"uestc_wifi.log").string())
        ->run_callback_for_default()
        ->capture_default_str();

    app.add_flag_callback(
        "--debug,-d",
        []() {
            spdlog::set_level(spdlog::level::debug);
        },
        "enable debug mode"
    );

    CLI11_PARSE(app, argc, argv);

    signal(SIGINT, exit_helper);
    signal(SIGTERM, exit_helper);
    helper->run();
}
