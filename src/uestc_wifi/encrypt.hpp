#pragma once
#include <string>

#include <hex.h>
#include <filters.h>

#include "uestc_wifi_helper_ns.hpp"


UESTC_WIFI_HELPER_NS_BEGIN(encrypt)

std::string base64_encode(std::string_view str);

std::string xencode(std::string_view str, std::string_view key);

template<typename HM>
requires requires (HM hm) {
    CryptoPP::HashFilter(hm, nullptr);
}
std::string encrypt(HM& hm, std::string_view str) {
    std::string digest;
    auto _ = CryptoPP::StringSource(
        (const CryptoPP::byte*)str.data(),
        str.size(),
        true,
        new CryptoPP::HashFilter(
            hm,
            new CryptoPP::HexEncoder(
                new CryptoPP::StringSink(digest),
                false
            )
        )
    );
    return digest;
}

UESTC_WIFI_HELPER_NS_END
