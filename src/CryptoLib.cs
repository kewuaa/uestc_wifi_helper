using System;
using System.Text;
using System.Collections.Generic;
using System.Security.Cryptography;

static class CryptoLib {
    static private int UnsignedRightShift(int x, int bit) => (int)((uint)x >> bit);
    static public string XEncode(string str, string key) {
        if (str == "") {
            return "";
        }
        Func<string, bool, List<int>> s = (a, b) => {
            var c = a.Length;
            List<int> v = new List<int>();
            for (int i = 0; i < c; i += 4) {
                int x = (int)a[i];
                for (int j = 1; j < 4; j++) {
                    if (i + j < c) {
                        x |= (int)a[i + j] << (j * 8);
                    } else break;
                }
                v.Add(x);
            }
            if (b) {
                v.Add(c);
            }
            return v;
        };
        Func<List<int>, string> l = (a) => {
            var d = a.Count;
            StringBuilder b = new StringBuilder();
            for (int i = 0; i < d; i++) {
                string v = $"{(char)(a[i] & 0xff)}{(char)(UnsignedRightShift(a[i], 8) & 0xff)}{(char)(UnsignedRightShift(a[i], 16) & 0xff)}{(char)(UnsignedRightShift(a[i], 24) & 0xff)}";
                b.Append(v);
            }
            return b.ToString();
        };
        (var v, var k) = (s(str, true), s(key, false));
        int k_len = k.Count < 4 ? 4 : k.Count;
        int n = v.Count - 1;
        int z = v[n];
        int y = v[0];
        int c = -1640531527;
        int m, e, p;
        double q = Math.Floor(6 + 52.0 / v.Count);
        int d = 0;
        while (0 < q--) {
            d = d + c & (-1);
            e = UnsignedRightShift(d, 2) & 3;
            for (p = 0; p < n; p++) {
                y = v[p + 1];
                m = UnsignedRightShift(z, 5) ^ y << 2;
                m += (UnsignedRightShift(y, 3) ^ z << 4) ^ (d ^ y);
                m += k[(int)((p & 3) ^ e)] ^ z;
                z = v[p] = v[p] + m & (-1);
            }
            y = v[0];
            m = UnsignedRightShift(z, 5) ^ y << 2;
            m += (UnsignedRightShift(y, 3) ^ z << 4) ^ (d ^ y);
            m += k[(int)((p & 3) ^ e)] ^ z;
            z = v[n] = v[n] + m & (-1);

        }
        return l(v);
    }

    static public string GetSHA1(string s) {
        SHA1 sha1 = SHA1.Create();
        var data = sha1.ComputeHash(Encoding.UTF8.GetBytes(s));
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < data.Length; i++) {
            b.Append(data[i].ToString("x2"));
        }
        return b.ToString();
    }

    static public string GetHMACMD5(string s, string key) {
        using HMACMD5 hmac = new HMACMD5(Encoding.UTF8.GetBytes(key));
        byte[] data = hmac.ComputeHash(Encoding.UTF8.GetBytes(s));
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < data.Length; i++) {
            b.Append(data[i].ToString("x2"));
        }
        return b.ToString();
    }

    static public class Base64 {
        static char _PADCHAR = '=';
        static string _ALPHA = "LVoJPiCN2R8G90yg+hmFHuacZ1OWMnrsSTXkYpUq/3dlbfKwv6xztjI7DeBE45QA";

        static public string Encode(string s) {
            if (s.Length == 0) {
                return s;
            }
            int i;
            int b10 = 0;
            int imax = s.Length - s.Length % 3;
            StringBuilder b = new StringBuilder();
            for (i = 0; i < imax; i+=3) {
                b10 = ((int)s[i] << 16) | ((int)s[i + 1] << 8) | ((int)s[i + 2]);
                b.Append($"{_ALPHA[b10 >> 18]}{_ALPHA[(b10 >> 12) & 63]}{_ALPHA[(b10 >> 6) & 63]}{_ALPHA[b10 & 63]}");
            }
            i = imax;
            switch (s.Length - imax) {
                case 1:
                    b10 = (int)s[i] << 16;
                    b.Append($"{_ALPHA[b10 >> 18]}{_ALPHA[(b10 >> 12) & 63]}{_PADCHAR}{_PADCHAR}");
                    break;
                case 2:
                    b10 = ((int)s[i] << 16) | ((int)s[i + 1] << 8);
                    b.Append($"{_ALPHA[b10 >> 18]}{_ALPHA[(b10 >> 12) & 63]}{_ALPHA[(b10 >> 6) & 63]}{_PADCHAR}");
                    break;
            }
            return b.ToString();
        }
    }
}
