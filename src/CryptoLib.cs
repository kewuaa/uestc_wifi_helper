using System.Text;
using System.Security.Cryptography;

namespace Kewuaa;

static class CryptoLib {
    static public string GetSHA1(string s) {
        SHA1 sha1 = SHA1.Create();
        var data = sha1.ComputeHash(Encoding.UTF8.GetBytes(s));
        StringBuilder b = new();
        for (int i = 0; i < data.Length; i++) {
            b.Append(data[i].ToString("x2"));
        }
        return b.ToString();
    }

    static public string GetHMACMD5(string s, string key) {
        using HMACMD5 hmac = new(Encoding.UTF8.GetBytes(key));
        byte[] data = hmac.ComputeHash(Encoding.UTF8.GetBytes(s));
        StringBuilder b = new();
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
            StringBuilder b = new();
            for (i = 0; i < imax; i+=3) {
                b10 = ((int)s[i] << 16) | ((int)s[i + 1] << 8) | ((int)s[i + 2]);
                b.Append($"{_ALPHA[b10 >> 18]}{_ALPHA[(b10 >> 12) & 63]}{_ALPHA[(b10 >> 6) & 63]}{_ALPHA[b10 & 63]}");
            }
            i = imax;
            if (s.Length - imax == 1) {
                b10 = (int)s[i] << 16;
                b.Append($"{_ALPHA[b10 >> 18]}{_ALPHA[(b10 >> 12) & 63]}{_PADCHAR}{_PADCHAR}");
            } else {
                b10 = ((int)s[i] << 16) | ((int)s[i + 1] << 8);
                b.Append($"{_ALPHA[b10 >> 18]}{_ALPHA[(b10 >> 12) & 63]}{_ALPHA[(b10 >> 6) & 63]}{_PADCHAR}");
            }
            return b.ToString();
        }
    }
}
