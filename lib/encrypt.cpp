#define EXPORT __declspec(dllexport) __cdecl
#include <iostream>
#include <math.h>
#include <string.h>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
typedef unsigned int uint32;


static
int unsigned_right_shift(uint32 x, int bit) {
    return (int)(x >> bit);
}

static
int left_shift(uint32 x, int bit) {
    return (int)(x << bit);
}


static
std::string x_encode(std::string str, std::string key) {
    if (str == "") {
        throw std::runtime_error("empty string");
    }
    auto s = [](std::string a, bool b) {
        auto c = a.length();
        std::vector<uint32> v;
        for (int i = 0; i < c; i += 4) {
            uint32 x = a[i];
            for (int j = 1; j < 4; j++) {
                if (i + j < c) {
                    x |= (uint32)a[i + j] << (j * 8);
                } else break;
            }
            v.push_back(x);
        }
        if (b) {
            v.push_back(c);
        }
        return v;
    };
    auto v = s(str, true);
    auto k = s(key, false);
    int k_len = k.size() < 4 ? 4 : k.size();
    int n = v.size() - 1;
    uint32 z = v[n];
    uint32 y = v[0];
    uint32 c = -1640531527;
    uint32 m, e, p;
    double q = floor(6 + 52.0 / v.size());
    uint32 d = 0;
    while (0 < q--) {
        d = d + c & (-1);
        e = d >> 2 & 3;
        for (p = 0; p < n; p++) {
            y = v[p + 1];
            m = z >> 5 ^ y << 2;
            m += (y >> 3 ^ z << 4) ^ (d ^ y);
            m += k[(p & 3) ^ e] ^ z;
            z = v[p] = v[p] + m & (-1);
        }
        y = v[0];
        m = z >> 5 ^ y << 2;
        m += (y >> 3 ^ z << 4) ^ (d ^ y);
        m += k[(int)((p & 3) ^ e)] ^ z;
        z = v[n] = v[n] + m & (-1);
    }

    const int len = v.size() * 4;
    std::unique_ptr<wchar_t> ss(new wchar_t[len + 1]);
    wchar_t* ptr = ss.get();
    for (int i = 0; i < v.size(); i++) {
        ptr[i * 4] = (wchar_t)(v[i] & 0xff);
        ptr[i * 4 + 1] = (wchar_t)(v[i] >> 8 & 0xff);
        ptr[i * 4 + 2] = (wchar_t)(v[i] >> 16 & 0xff);
        ptr[i * 4 + 3] = (wchar_t)(v[i] >> 24 & 0xff);
    }
    ptr[v.size() * 4] = '\0';

    // base64 encode part
    const char* PAD = "=";
    const std::string ALPHA = "LVoJPiCN2R8G90yg+hmFHuacZ1OWMnrsSTXkYpUq/3dlbfKwv6xztjI7DeBE45QA";
    int i;
    uint32 b10 = 0;
    int imax = len - len % 3;
    std::ostringstream b;
    for (i = 0; i < imax; i += 3) {
        b10 = ((uint32)ptr[i] << 16) | ((uint32)ptr[i + 1] << 8) | ((uint32)ptr[i + 2]);
        b << ALPHA[b10 >> 18];
        b << ALPHA[(b10 >> 12) & 63];
        b << ALPHA[(b10 >> 6) & 63];
        b << ALPHA[b10 & 63];
    }
    i = imax;
    switch (len - imax) {
        case 1:
            b10 = (uint32)ptr[i] << 16;
            b << ALPHA[b10 >> 18];
            b << ALPHA[(b10 >> 12) & 63];
            b << PAD;
            b << PAD;
            break;
        case 2:
            b10 = ((uint32)ptr[i] << 16) | ((uint32)ptr[i + 1] << 8);
            b << ALPHA[b10 >> 18];
            b << ALPHA[(b10 >> 12) & 63];
            b << ALPHA[(b10 >> 6) & 63];
            b << PAD;
            break;
    }
    return b.str();
}


extern "C" {
    EXPORT char* xencode(char* str, char* key) {
        auto encoded_str = x_encode(str, key);
        std::cout << encoded_str << std::endl;
        char* ptr = new char[encoded_str.length() + 1];
        strcpy(ptr, encoded_str.c_str());
        return ptr;
    }

    EXPORT void ptr_free(char* ptr) {
        delete[] ptr;
    }
}
