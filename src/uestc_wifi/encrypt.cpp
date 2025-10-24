#include <cmath>
#include <vector>
#include <cstdint>
#include <stdexcept>

#include "encrypt.hpp"


namespace {

constexpr char PAD = '=';
constexpr const char* ALPHA = "LVoJPiCN2R8G90yg+hmFHuacZ1OWMnrsSTXkYpUq/3dlbfKwv6xztjI7DeBE45QA";

std::vector<uint32_t> s(std::string_view a, bool b) {
    std::vector<uint32_t> v;
    v.reserve(a.size()/4+2);
    for (size_t i = 0; i < a.size(); i+=4) {
        uint32_t x = 0;
        for (int j = 0; j < 4; ++j) {
            if (i+j < a.size()) {
                x |= (static_cast<uint32_t>(a[i+j]) << (j*8));
            } else break;
        }
        v.push_back(x);
    }
    if (b) {
        v.push_back(static_cast<uint32_t>(a.size()));
    }
    return v;
}

}


UESTC_WIFI_HELPER_NS_BEGIN(encrypt)

std::string base64_encode(std::string_view str) {
    size_t i = 0;
    uint32_t b10 = 0;
    auto imax = str.size() - str.size() % 3;
    std::string res;
    for (; i < imax; i+=3) {
        b10 = (((uint32_t)(uint8_t)str[i]<<16) | ((uint32_t)(uint8_t)str[i+1])<<8) | ((uint32_t)(uint8_t)str[i+2]);
        res.push_back(ALPHA[b10 >> 18]);
        res.push_back(ALPHA[b10 >> 12 & 63]);
        res.push_back(ALPHA[b10 >> 6 & 63]);
        res.push_back(ALPHA[b10 & 63]);
    }
    i = imax;
    switch (str.size()-imax) {
        case 1: {
            b10 = (uint32_t)(uint8_t)str[i] << 16;
            res.push_back(ALPHA[b10 >> 18]);
            res.push_back(ALPHA[b10 >> 12 & 63]);
            res.push_back(PAD);
            res.push_back(PAD);
            break;
        }
        case 2: {
            b10 = (((uint32_t)(uint8_t)str[i]<<16) | ((uint32_t)(uint8_t)str[i+1])<<8);
            res.push_back(ALPHA[b10 >> 18]);
            res.push_back(ALPHA[b10 >> 12 & 63]);
            res.push_back(ALPHA[b10 >> 6 & 63]);
            res.push_back(PAD);
            break;
        }
    }
    return res;
}

std::string xencode(std::string_view str, std::string_view key) {
    if (str.empty()) {
        throw std::runtime_error("empty string");
    }

    auto v = s(str, true);
    auto k = s(key, false);
    if (k.size() < 4) {
        k.resize(4, 0);
    }

    int n = static_cast<int>(v.size()) - 1;
    if (n < 0) {
        return ""; // 异常处理（理论上str非空时v至少含1个元素）
    }

    uint32_t z = v[n];
    uint32_t y = v[0];
    const uint32_t c = 0x9E3779B9; // 常量（原代码位或结果）
    int q = static_cast<int>(floor(6.0 + 52.0 / (n + 1))); // 计算循环次数
    uint32_t d = 0;

    // 主加密循环
    while (q-- > 0) {
        d = (d + c) & 0xFFFFFFFF; // 确保32位无符号
        int e = (d >> 2) & 3; // 取d的第2-3位（0-3范围）

        // 处理数组前n个元素
        for (int p = 0; p < n; ++p) {
            y = v[p + 1];
            // 计算m值（核心混淆逻辑）
            uint32_t m = (z >> 5) ^ (y << 2);
            m += ((y >> 3) ^ (z << 4)) ^ (d ^ y);
            m += k[(p & 3) ^ e] ^ z;
            // 更新当前元素和z
            z = v[p] = (v[p] + m) & 0xFFFFFFFF;
        }

        // 处理数组最后一个元素
        y = v[0];
        uint32_t m = (z >> 5) ^ (y << 2);
        m += ((y >> 3) ^ (z << 4)) ^ (d ^ y);
        m += k[(n & 3) ^ e] ^ z;
        z = v[n] = (v[n] + m) & 0xFFFFFFFF;
    }

    return { (char*)v.data(), v.size()*sizeof(uint32_t)/sizeof(char) };

}

UESTC_WIFI_HELPER_NS_END
