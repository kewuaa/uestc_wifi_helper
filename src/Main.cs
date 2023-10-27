using System.Text;
using System.Diagnostics;
using System.Runtime.InteropServices;

using Nett;

public partial class Program {
    [LibraryImport("user32.dll", EntryPoint = "MessageBoxW", StringMarshalling = StringMarshalling.Utf16)]
    static private partial int MessageBox(
        int hWnd,
        [MarshalAs(UnmanagedType.LPWStr)]
        string text,
        [MarshalAs(UnmanagedType.LPWStr)]
        string title,
        uint t
    );

    [STAThread]
    static public void Main() {
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
                # 默认为电信（移动未测试过，不知道能不能用）
                network_operator = "dx"

                # 间隔多长时间检查一次网络
                # 单位为小时
                # 设置为非正数时相当于单独检查一次网络
                # 默认为 6，即每隔 6 小时自动检查一次网络
                check_interval = 6
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
        var network_operator = config.TryGetValue("network_operator")?.Get<string>();
        var check_interval = config.TryGetValue("check_interval")?.Get<int>() ?? 6;

        Kewuaa.UESTCWIFIHelper helper = new(username.Get<string>(), password.Get<string>(), network_operator);
        if (check_interval <= 0) {
            try {
                var status = helper.Check().Result;
                string text = status switch {
                    Kewuaa.UESTCWIFIHelper.CheckedStatus.StillOnline => "设备已在线",
                    Kewuaa.UESTCWIFIHelper.CheckedStatus.NotConnected => "未连接WiFi或网线",
                    Kewuaa.UESTCWIFIHelper.CheckedStatus.DeviceWithinScope => "设备不在范围内",
                    Kewuaa.UESTCWIFIHelper.CheckedStatus.SuccessfullyLogin => "登录WiFi成功",
                };
                MessageBox(0, text, "提示", 0x00000000);
            } catch (Exception e) {
                MessageBox(0, e.ToString(), "错误", 0x00000000);
            }
        } else {
            Application.Run(new UESTCWIFIHelper.MainForm(helper, check_interval));
        }
    }
}