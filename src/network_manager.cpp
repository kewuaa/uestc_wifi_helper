#include <sys/epoll.h>

#include "network_manager.hpp"




UESTC_WIFI_HELPER_NS_BEGIN(nm)

std::unique_ptr<sdbus::IProxy> create_proxy(sdbus::IConnection& conn) {
    return sdbus::createProxy(conn, sdbus::ServiceName(NM_SERVICE), sdbus::ObjectPath(NM_PATH));
}

UESTC_WIFI_HELPER_NS_END
