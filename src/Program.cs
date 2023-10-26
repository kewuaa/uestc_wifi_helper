// See https://aka.ms/new-console-template for more information
using System.Net;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Runtime.InteropServices;
using System.Diagnostics;

using Nett;

namespace Kewuaa;

[JsonSerializable(typeof(UESTCWIFIHelper.ResponseDict))]
internal partial class ResponseDictSourceGenerationContext: JsonSerializerContext {}


[JsonSourceGenerationOptions(WriteIndented = false)]
[JsonSerializable(typeof(UESTCWIFIHelper.LoginData))]
internal partial class LoginDataSourceGenerationContext: JsonSerializerContext {}


static partial class UESTCWIFIHelper {
    public class DeviceWithinScopeException: Exception {
        public DeviceWithinScopeException(): base("The device is not within the scope of certification") {}
    }

    public class ResponseDict {
        public string? error {get; set;}
        public string? domain {get; set;}
        public string? error_msg {get; set;}
        public string? user_name {get; set;}
        public string? online_ip {get; set;}
        public string? client_ip {get; set;}
        public string? challenge {get; set;}
    }

    public class LoginData {
        public string? username {get; set;}
        public string? password {get; set;}
        public string? ip {get; set;}
        public string? acid {get; set;}
        public string? enc_ver {get; set;}
    }

    static private string callback_string = "jQuery112409729861590799633_1698107269291";

    [Conditional("DEBUG")]
    static private void Log(string s) => Console.WriteLine(s);

    [LibraryImport("kernel32.dll")]
    static private partial IntPtr GetConsoleWindow();

    [LibraryImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    static private partial bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [LibraryImport("user32.dll", EntryPoint = "MessageBoxW", StringMarshalling = StringMarshalling.Utf16)]
    static private partial int MessageBox(
        int hWnd,
        [MarshalAs(UnmanagedType.LPWStr)]
        string text,
        [MarshalAs(UnmanagedType.LPWStr)]
        string title,
        uint t
    );

    static private long GetTimeStamp() => new DateTimeOffset(DateTime.UtcNow).ToUnixTimeMilliseconds();

    static private ResponseDict ParseResponse(HttpResponseMessage res) {
        res.EnsureSuccessStatusCode();
        var res_text = res.Content.ReadAsStringAsync().Result;
        if (res_text.Substring(0, callback_string.Length) != callback_string) {
            throw new Exception("Error response message");
        }
        string json_str = res_text.Substring(callback_string.Length + 1, res_text.Length - callback_string.Length - 2);
        var json = JsonSerializer.Deserialize<ResponseDict>(json_str, ResponseDictSourceGenerationContext.Default.ResponseDict);
        if (json is null) {
            throw new Exception("Deserialize json string failed");
        }
        return json;
    }

    static private bool CheckConnect(HttpClient client) {
        string url = "http://10.253.0.235/srun_portal_success?ac_id=3&theme=yd";
        HttpRequestMessage request = new(HttpMethod.Get, url);
        var res = client.Send(request);
        try {
            res.EnsureSuccessStatusCode();
        } catch (System.Net.Http.HttpRequestException) {
            Log("Not connected to network cable or WiFi");
            return false;
        }
        return true;
    }

    static private (bool, string) CheckOnline(HttpClient client) {
        var time = GetTimeStamp();
        string url = $"http://10.253.0.235/cgi-bin/rad_user_info?callback={callback_string}&_={time}";
        HttpRequestMessage request = new(HttpMethod.Get, url);
        var res = client.Send(request);
        var res_dict = ParseResponse(res);
        string msg = res_dict.error!;
        switch (msg) {
            case "ok":
                string online_ip = res_dict.online_ip!;
                Log($"\"{online_ip}\" user: {res_dict.user_name}@{res_dict.domain} already online");
                return (true, res_dict.online_ip!);
            case "not_online_error":
                string client_ip = res_dict.client_ip!;
                Log($"\"{client_ip}\" not online");
                return (false, client_ip);
            default:
                throw new Exception("Got unexpected msg");
        }
    }

    static private void Login(
        HttpClient client,
        string username,
        string password,
        string network_operator,
        string client_ip
    ) {
        LoginData data = new() {username = $"{username}@{network_operator}", password = password, ip = client_ip, acid = "3", enc_ver = "srun_bx1"};
        string url = $"http://10.253.0.235/cgi-bin/get_challenge?callback={callback_string}&username={data.username}&ip={client_ip}&_={GetTimeStamp()}";
        HttpRequestMessage request = new(HttpMethod.Get, url);
        var res = client.Send(request);
        var res_dict = ParseResponse(res);
        var ok = res_dict.error == "ok";
        if (!ok) {
            throw new Exception($"Fetch challenge failed: {res_dict.error_msg}");
        }
        var token = res_dict.challenge!;
        var str = JsonSerializer.Serialize<LoginData>(data, LoginDataSourceGenerationContext.Default.LoginData);
        string info = $"{{SRBX1}}{CryptoLib.Base64.Encode(CryptoLib.XEncode(str, token))}";
        string password_md5 = CryptoLib.GetHMACMD5(password, token);
        string chksum_str = $"{token}{data.username}{token}{password_md5}{token}3{token}{client_ip}{token}200{token}1{token}{info}";
        string chksum = CryptoLib.GetSHA1(chksum_str);
        url = "http://10.253.0.235/cgi-bin/srun_portal?"
            + $"callback={callback_string}&action=login&username={data.username}&password=%7BMD5%7D{password_md5}&"
            + $"ac_id=3&ip={client_ip}&chksum={chksum}&info={WebUtility.UrlEncode(info)}&"
            + $"n=200&type=1&os=Windows%2010&name=Windows&double_stack=0&_={GetTimeStamp()}";
        request = new(HttpMethod.Get, url);
        res = client.Send(request);
        res_dict = ParseResponse(res);
        ok = res_dict.error == "ok";
        if (!ok) {
            if (res_dict.error_msg == "INFO Error锛宔rr_code=2") {
                Log("The device is not within the scope of certification");
                throw new DeviceWithinScopeException();
            }
            throw new Exception($"Login failed: {res_dict.error_msg}");
        }
        Log($"\"{client_ip}\" user: {data.username} successfully login");
    }

    static public void Main() {
#if !DEBUG
        // hide console window
        var hwnd = GetConsoleWindow();
        ShowWindow(hwnd, 0);
#endif
        var home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        var config_file = Path.Combine(home, "uestc_wifi.toml");
        if (!File.Exists(config_file)) {
            using FileStream fs = new(config_file, FileMode.Create);
            fs.Write(Encoding.UTF8.GetBytes(
                """
                username = "your username"

                password = "your password"

                # network operator of your wifi
                # 电信: dx
                # 移动: cmcc
                # 默认为电信
                network_operator = "dx"
                """
            ));
            MessageBox(0, $"你需要在 {config_file} 中保存你的用户名和密码等配置", "提醒", 0x00000000);
            Process p = new();
            p.StartInfo.FileName = "notepad";
            p.StartInfo.Arguments = config_file;
            p.Start();
            return;
        }
        var config = Toml.ReadFile(config_file);
        var username = config.TryGetValue("username");
        if (username is null) {
            MessageBox(0, $"未在 {config_file} 中读取到 username", "警告", 0x00000000);
            return;
        }
        var password = config.TryGetValue("password");
        if (password is null) {
            MessageBox(0, $"未在 {config_file} 中读取到 password", "警告", 0x00000000);
            return;
        }
        var network_operator = config.TryGetValue("network_operator");

        var handle = new SocketsHttpHandler();
        handle.CookieContainer.Add(new Cookie("lang", "zh-CN") {Domain = "10.253.0.235"});
        HttpClient client = new(handle);
        client.DefaultRequestHeaders.Add("Host", "10.253.0.235");
        client.DefaultRequestHeaders.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36");
        if (!CheckConnect(client)) {
            return;
        }
        var (online, ip) = CheckOnline(client);
        if (!online) {
            try {
                Login(client, username.ToString()!, password.ToString()!, network_operator.ToString() ?? "dx", ip);
            } catch (DeviceWithinScopeException) {
                return;
            } catch (Exception e) {
                MessageBox(0, e.ToString(), "错误", 0x00000000);
            }
        }
    }
}
