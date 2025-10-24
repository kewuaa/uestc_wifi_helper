#pragma once
#include <string>
#include <ranges>

#include <magic_enum/magic_enum.hpp>

#include "uestc_wifi_helper_ns.hpp"
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

UESTC_WIFI_HELPER_NS_END
