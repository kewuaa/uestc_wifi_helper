#include <CLI/CLI.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Validators.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <battery/embed.hpp>

#include "utils.hpp"
#include "uestc_wifi_helper.hpp"
using namespace UESTC_WIFI_HELPER_NS;


int main(int argc, char** argv) {
    auto home = utils::home_path();
    std::string config_path { (home/"uestc_wifi.toml").string() };

    CLI::App app;
    argv = app.ensure_utf8(argv);

    app.description(std::format("config path: [{}]", config_path));

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

    CLI11_PARSE(app, argc, argv);

    if (!fs::exists(config_path)) {
        {
            std::ofstream f(config_path);
            f << b::embed<"template.toml">().str();
        }
        if (!utils::open_with_default_app(config_path)) {
            SPDLOG_ERROR("failed to open default config file");
            exit(1);
        }
    }
    set_signal_handle();
    UESTCWifiHelper::init(config_path).run();
}
