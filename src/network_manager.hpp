#pragma once
#include <utility>

#include "sdbus-c++/sdbus-c++.h"
#include "uestc_wifi_helper_ns.hpp"


UESTC_WIFI_HELPER_NS_BEGIN(nm)

std::unique_ptr<sdbus::IProxy> create_proxy(sdbus::IConnection& conn);

UESTC_WIFI_HELPER_NS_END
