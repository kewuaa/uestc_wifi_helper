#include <chrono>
#include <utility>

#include <sha.h>
#include <md5.h>
#include <hmac.h>
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "encrypt.hpp"
#include "uestc_wifi.hpp"
using json = nlohmann::ordered_json;


namespace {
using namespace std::chrono;
using namespace uestc_wifi_helper;
#define CHECK_RESULT(result) if (!result) {                                \
    SPDLOG_ERROR("httplib error: {}", httplib::to_string(result.error())); \
    return std::unexpected(UESTCWifi::Error::HttplibError);                \
}

constexpr const char CALLBACK_STRING[] = "jQuery112409729861590799633_1698107269291";

std::string target_url(UESTCWifi::NetworkOperator op) {
    std::string url;
    switch (op) {
        case UESTCWifi::NetworkOperator::CTCC:
        case UESTCWifi::NetworkOperator::CMCC: {
            url = "http://10.253.0.235";
            break;
        }
        case UESTCWifi::NetworkOperator::UESTC:
        case UESTCWifi::NetworkOperator::CTCC_UESTC: {
            url = "http://10.253.0.237";
            break;
        }
    }
    return url;
}

std::string timestamp() {
    auto now = system_clock::now();
    auto dur = now.time_since_epoch();
    return std::to_string(duration_cast<milliseconds>(dur).count());
}

auto response(std::string_view body) {
    if (!body.starts_with(CALLBACK_STRING)) {
        throw std::runtime_error("invalid response");
    }
    auto json = json::parse(
        body.substr(
            sizeof(CALLBACK_STRING),
            body.size()-sizeof(CALLBACK_STRING)-1
        )
    );
    return json;
}

}


UESTC_WIFI_HELPER_NS_BEGIN()

struct UESTCWifi::impl {
    std::string username;
    std::string password;
    std::string acid;
    std::string network_operator;
    httplib::Headers headers;
    httplib::Client client;

    impl(std::string_view username, std::string_view password, NetworkOperator op):
        username(username), password(password), client(target_url(op))
    {
        std::string target_ip;
        switch (op) {
            case NetworkOperator::CTCC: {
                acid = '3';
                network_operator = "dx";
                target_ip = "10.253.0.235";
                break;
            }
            case NetworkOperator::CMCC: {
                acid = '3';
                network_operator = "cmcc";
                target_ip = "10.253.0.235";
                break;
            }
            case NetworkOperator::UESTC: {
                acid = '1';
                network_operator = "dx-uestc";
                target_ip = "10.253.0.237";
                break;
            }
            case NetworkOperator::CTCC_UESTC: {
                acid = '1';
                network_operator = "dx";
                target_ip = "10.253.0.237";
                break;
            }
        }
        headers.emplace("Content-Type", "application/x-www-form-urlencoded");
        headers.emplace("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36");
        headers.emplace("Host", target_ip);
        headers.emplace("Cookie", "lang=zh-CN");
    }

    Result<CheckOnlineState> check_online() {
        httplib::Params params {
            { "callback", CALLBACK_STRING },
            { "_", timestamp() },
        };
        auto res = client.Get("/cgi-bin/rad_user_info", params, headers);
        CHECK_RESULT(res);
        auto resp = response(res->body);
        auto error = resp["error"].get<std::string>();
        std::string ip = resp["online_ip"].is_null() ? "" : resp["online_ip"].get<std::string>();
        if (error == "ok") {
            return CheckOnlineState(true, ip);
        } else if (error == "not_online_error") {
            return CheckOnlineState(false, ip);
        } else if (error == "speed_limit_error") {
            return std::unexpected(Error::AuthRequestsFrequently);
        } else {
            throw std::runtime_error("Got unexpected message from response when checking online status");
        }
    }

    Result<std::string> challenge(std::string_view client_ip) {
        httplib::Params params {
            { "callback", CALLBACK_STRING },
            { "username", std::format("{}@{}", username, network_operator) },
            { "ip", std::string(client_ip) },
            { "_", timestamp() },
        };
        auto res = client.Get("/cgi-bin/get_challenge", params, headers);
        CHECK_RESULT(res);
        auto resp = response(res->body);
        if (resp["error"] != "ok") {
            throw std::runtime_error(
                std::format("Fetch challenge failed: {}", resp["error_msg"].get<std::string>())
            );
        }
        assert(!resp["challenge"].is_null() && "Challenge is null");
        return resp["challenge"].get<std::string>();
    }

    Result<void> login(std::string_view client_ip) {
        return challenge(client_ip).and_then([this, client_ip](std::string token) -> Result<void> {
            auto username_ = std::format("{}@{}", username, network_operator);

            json data;
            data["username"] = username_;
            data["password"] = password;
            data["ip"] = client_ip;
            data["acid"] = acid;
            data["enc_ver"] = "srun_bx1";
            auto info = "{SRBX1}"
                + encrypt::base64_encode(encrypt::xencode(data.dump(), token));

            auto hmacmd5 = CryptoPP::HMAC<CryptoPP::MD5>((const CryptoPP::byte*)token.data(), token.size());
            auto password_md5 = encrypt::encrypt(hmacmd5, password);

            auto chksum_str = token + username_
                + token + password_md5
                + token + acid
                + token + std::string(client_ip)
                + token + "200"
                + token + '1'
                + token + info
            ;
            auto sha1 = CryptoPP::SHA1();
            auto chksum = encrypt::encrypt(sha1, chksum_str);

            httplib::Params params {
                { "callback", CALLBACK_STRING },
                { "action", "login" },
                { "username", username_ },
                { "password", "{MD5}"+password_md5 },
                { "ac_id", acid },
                { "ip", std::string(client_ip) },
                { "chksum", chksum },
                { "info", info },
                { "n", "200" },
                { "type", "1" },
                { "os", "Linux" },
                { "name", "Linux" },
                { "double_stack", "0" },
                { "_", timestamp() },
            };
            auto res = client.Get("/cgi-bin/srun_portal", params, headers);
            CHECK_RESULT(res);

            auto resp = response(res->body);
            auto error = resp["error"];
            if (error != "ok") {
                auto error_msg = resp["error_msg"].get<std::string>();
                if (error_msg == "INFO Error锛宔rr_code=2") {
                    return std::unexpected(Error::DeviceWithinScope);
                } else if (error_msg == "E2901: (Third party 1)bind_user2: ldap_bind error" || error_msg == "E2901: (Third party 1)ldap_first_entry error") {
                    return std::unexpected(Error::IncorrectUsernameOrPassword);
                } else if (error_msg == "CHALLENGE failed, BAS respond timeout.") {
                    return std::unexpected(Error::NetworkConnectionTimeout);
                } else {
                    throw new std::runtime_error(std::format(
                        "login failed: error: {}, error_msg: {}",
                        error.get<std::string>(),
                        error_msg
                    ));
                }
            }
            return {};
        });
    }
};


UESTCWifi::UESTCWifi(UESTCWifi&& wifi): pimpl_(std::exchange(wifi.pimpl_, nullptr)) {
    //
}


UESTCWifi::UESTCWifi(std::string_view username, std::string_view password, NetworkOperator op): pimpl_(new impl(username, password, op)) {
    //
}

UESTCWifi::~UESTCWifi() {
    if (auto ptr = std::exchange(pimpl_, nullptr); ptr) {
        delete ptr;
    }
}

UESTCWifi::Result<UESTCWifi::CheckOnlineState> UESTCWifi::check_online() const {
    return pimpl_->check_online();
}

UESTCWifi::Result<void> UESTCWifi::login(std::string_view client_ip) const {
    return pimpl_->login(client_ip);
}

UESTC_WIFI_HELPER_NS_END
