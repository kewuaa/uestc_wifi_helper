#pragma once
#include <string>
#include <ranges>
#include <filesystem>

#include <magic_enum/magic_enum.hpp>

#include "uestc_wifi_helper_ns.hpp"
namespace fs = std::filesystem;
using namespace std::ranges;


UESTC_WIFI_HELPER_NS_BEGIN(utils)

template<typename T>
std::string join_enum() {
    std::string res;
    for (auto c : magic_enum::enum_names<T>()|views::join_with(',')) {
        res.push_back(c);
    }
    return res;
}

inline fs::path home_path() {
    return std::getenv(
#ifdef __linux__
        "HOME"
#elifdef _WIN32
        "USERPROFILE"
#endif
    );
}

inline bool open_with_default_app(std::string_view path) {
    return std::system(
        std::format(
#ifdef __linux__
            "xdg-open"
#elifdef _WIN32
            "notepad.exe"
#endif
            " \"{}\"",
            path
        ).c_str()
    ) == 0;
}

UESTC_WIFI_HELPER_NS_END
