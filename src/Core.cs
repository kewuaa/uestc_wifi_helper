﻿// See https://aka.ms/new-console-template for more information
using System.Net;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Diagnostics;

namespace Kewuaa;

[JsonSerializable(typeof(UESTCWIFIHelper.ResponseDict))]
internal partial class ResponseDictSourceGenerationContext: JsonSerializerContext {}


[JsonSourceGenerationOptions(WriteIndented = false)]
[JsonSerializable(typeof(UESTCWIFIHelper.LoginData))]
internal partial class LoginDataSourceGenerationContext: JsonSerializerContext {}


public class UESTCWIFIHelper {
    public class DeviceWithinScopeException: Exception {
        public DeviceWithinScopeException(): base("The device is not within the scope of certification") {}
    }

    public enum CheckedStatus {
        StillOnline,
        NotConnected,
        DeviceWithinScope,
        SuccessfullyLogin,
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

    static private string _callback_string = "jQuery112409729861590799633_1698107269291";

    [Conditional("DEBUG")]
    static private void Log(string s) => Console.WriteLine(s);

    static private long GetTimeStamp() => new DateTimeOffset(DateTime.UtcNow).ToUnixTimeMilliseconds();

    static private async Task<ResponseDict> ParseResponse(HttpResponseMessage res) {
        res.EnsureSuccessStatusCode();
        var res_text = await res.Content.ReadAsStringAsync();
        if (res_text.Substring(0, _callback_string.Length) != _callback_string) {
            throw new Exception("Error response message");
        }
        string json_str = res_text.Substring(_callback_string.Length + 1, res_text.Length - _callback_string.Length - 2);
        var json = JsonSerializer.Deserialize<ResponseDict>(json_str, ResponseDictSourceGenerationContext.Default.ResponseDict);
        if (json is null) {
            throw new Exception("Deserialize json string failed");
        }
        return json;
    }

    private HttpClient _client;
    private string _username;
    private string _password;
    private string _network_operator;

    private async Task<bool> CheckConnect() {
        string url = "http://10.253.0.235/srun_portal_success?ac_id=3&theme=yd";
        HttpRequestMessage request = new(HttpMethod.Get, url);
        var res = await _client.SendAsync(request);
        try {
            res.EnsureSuccessStatusCode();
        } catch (System.Net.Http.HttpRequestException) {
            Log("Not connected to network cable or WiFi");
            return false;
        }
        return true;
    }

    private async Task<(bool, string)> CheckOnline() {
        var time = GetTimeStamp();
        string url = $"http://10.253.0.235/cgi-bin/rad_user_info?callback={_callback_string}&_={time}";
        HttpRequestMessage request = new(HttpMethod.Get, url);
        var res = await _client.SendAsync(request);
        var res_dict = await ParseResponse(res);
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

    private async Task Login(string client_ip) {
        LoginData data = new() {username = $"{_username}@{_network_operator}", password = _password, ip = client_ip, acid = "3", enc_ver = "srun_bx1"};
        string url = $"http://10.253.0.235/cgi-bin/get_challenge?callback={_callback_string}&username={data.username}&ip={client_ip}&_={GetTimeStamp()}";
        HttpRequestMessage request = new(HttpMethod.Get, url);
        var res = await _client.SendAsync(request);
        var res_dict = await ParseResponse(res);
        var ok = res_dict.error == "ok";
        if (!ok) {
            throw new Exception($"Fetch challenge failed: {res_dict.error_msg}");
        }
        var token = res_dict.challenge!;
        var str = JsonSerializer.Serialize<LoginData>(data, LoginDataSourceGenerationContext.Default.LoginData);
        string info = $"{{SRBX1}}{CryptoLib.Base64.Encode(CryptoLib.XEncode(str, token))}";
        string password_md5 = CryptoLib.GetHMACMD5(_password, token);
        string chksum_str = $"{token}{data.username}{token}{password_md5}{token}3{token}{client_ip}{token}200{token}1{token}{info}";
        string chksum = CryptoLib.GetSHA1(chksum_str);
        url = "http://10.253.0.235/cgi-bin/srun_portal?"
            + $"callback={_callback_string}&action=login&username={data.username}&password=%7BMD5%7D{password_md5}&"
            + $"ac_id=3&ip={client_ip}&chksum={chksum}&info={WebUtility.UrlEncode(info)}&"
            + $"n=200&type=1&os=Windows%2010&name=Windows&double_stack=0&_={GetTimeStamp()}";
        request = new(HttpMethod.Get, url);
        res = await _client.SendAsync(request);
        res_dict = await ParseResponse(res);
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

    public async Task<CheckedStatus> Check() {
        if (!await CheckConnect()) {
            return CheckedStatus.NotConnected;
        }
        var (online, ip) = await CheckOnline();
        if (online) return CheckedStatus.StillOnline;
        try {
            await Login(ip);
            return CheckedStatus.SuccessfullyLogin;
        } catch (DeviceWithinScopeException) {
            return CheckedStatus.DeviceWithinScope;
        }
    }

    public UESTCWIFIHelper(string username, string password, string? network_operator) {
        var handle = new SocketsHttpHandler();
        handle.CookieContainer.Add(new Cookie("lang", "zh-CN") {Domain = "10.253.0.235"});
        _client = new(handle);
        _client.DefaultRequestHeaders.Add("Host", "10.253.0.235");
        _client.DefaultRequestHeaders.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36");
        _username = username;
        _password = password;
        _network_operator = network_operator ?? "dx";
    }
}