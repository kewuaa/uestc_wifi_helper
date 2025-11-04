#pragma once
#include <string>
#include <expected>

#include <magic_enum/magic_enum.hpp>

#include "uestc_wifi_export.hpp"
#include "uestc_wifi_helper_ns.hpp"


UESTC_WIFI_HELPER_NS_BEGIN()

class UESTC_WIFI_EXPORT UESTCWifi {
public:
    enum class NetworkOperator {
        // 寝室电信
        CTCC,
        // 寝室移动
        CMCC,
        // 教研室
        UESTC,
        // 教研室电信
        CTCC_UESTC,
    };

    enum class Error {
        HttplibError,
        HttpRedirect,
        IncorrectUsernameOrPassword,
        DeviceWithinScope,
        NetworkConnectionTimeout,
        AuthRequestsFrequently,
    };
    static constexpr const char* translate_error(Error err) {
        constexpr const char* infos[] = {
            "httplib error",
            "http request triggers redirect",
            "Incorrect username or password",
            "The device is not within the scope of certification",
            "Network connection timed out, please try again later",
            "Authorization request frequently",
        };
        return infos[*magic_enum::enum_index(err)];
    }

    template<typename T>
    using Result = std::expected<T, Error>;

    struct CheckOnlineState {
        bool online;
        std::string ip;
    };

    UESTCWifi(UESTCWifi&&);
    UESTCWifi(const UESTCWifi&) = delete;
    UESTCWifi(std::string_view user_name, std::string_view password, NetworkOperator op);
    ~UESTCWifi();
    Result<void> login(std::string_view client_ip) const;
    Result<CheckOnlineState> check_online() const;
private:
    struct impl;
    impl* pimpl_ { nullptr };
};

UESTC_WIFI_HELPER_NS_END
