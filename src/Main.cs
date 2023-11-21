using System;
using System.IO;
using System.Text;
using System.Diagnostics;
using System.Windows.Forms;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

using Nett;

public static class Program {
    [DllImport("user32.dll", EntryPoint = "MessageBoxW", CharSet = CharSet.Auto)]
    static private extern int MessageBox(
        int hWnd,
        string text,
        string title,
        uint t
    );

    [STAThread]
    static public async Task Main() {
        var home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        var config_file = Path.Combine(home, "uestc_wifi.toml");
        var log_file = Path.Combine(home, "uestc_wifi.log");
        if (!File.Exists(config_file)) {
            using FileStream fs = new FileStream(config_file, FileMode.Create);
            var content = Encoding.UTF8.GetBytes(
@"
username = ""your username""

password = ""your password""

# network operator of your wifi
# 寝室电信网: 0
# 寝室移动网: 1
# 教研室网: 2
# 教研室电信网: 3
# 默认为寝室电信网
network_operator = 0

# 后台程序检查网络状况的间隔时长
# 单位为秒，整数
# 设置为非正数时程序单独运行一次
check_interval = -1
"
            );
            fs.Write(content, 0, content.Length);
            MessageBox(0, $"你需要在 {config_file} 中保存你的用户名和密码等配置", "提醒", 0x00000000);
            Process p = new Process();
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
        UESTCWIFIHelper.NetworkOperator network_operator;
        switch (config.TryGetValue("network_operator")?.Get<int>() ?? 0) {
            case 0:
                network_operator = UESTCWIFIHelper.NetworkOperator.CTCC;
                break;
            case 1:
                network_operator = UESTCWIFIHelper.NetworkOperator.CMCC;
                break;
            case 2:
                network_operator = UESTCWIFIHelper.NetworkOperator.UESTC;
                break;
            case 3:
                network_operator = UESTCWIFIHelper.NetworkOperator.CTCC_UESTC;
                break;
            default:
                MessageBox(0, $"{config_file} 中的配置项 `network_operator` 不合法", "警告", 0x00000000);
                return;
        }
        var check_interval = config.TryGetValue("check_interval")?.Get<int>() ?? 30;

        var helper = new UESTCWIFIHelper(
            username.Get<string>(),
            password.Get<string>(),
            network_operator
        );

        if (check_interval <= 0) {
            byte[] log_content;
            using FileStream log_fs = File.Exists(log_file) ? new FileStream(log_file, FileMode.Append) : new FileStream(log_file, FileMode.Create);
            var date = DateTime.Now.ToLocalTime().ToString();
            try {
                UESTCWIFIHelper.CheckedStatus status;
                try {
                    status = await helper.Check();
                } catch (UESTCWIFIHelper.NotConnectedException) {
                    MessageBox(0, "未连接WiFi或网线", "警告", 0x00000000);
                    log_content = Encoding.UTF8.GetBytes($"{date} - WARNING: 未连接WiFi或网线\n");
                    goto log_and_exit;
                }
                string text = status switch {
                    UESTCWIFIHelper.CheckedStatus.StillOnline => "设备已在线",
                    UESTCWIFIHelper.CheckedStatus.DeviceWithinScope => "设备不在范围内",
                    UESTCWIFIHelper.CheckedStatus.SuccessfullyLogin => "登录WiFi成功",
                    _ => throw new Exception("unreachable"),
                };
                log_content = Encoding.UTF8.GetBytes($"{date} - INFO: {text}\n");
            } catch (UESTCWIFIHelper.IncorrectUsernameOrPasswordException) {
                MessageBox(0, "账号或密码错误", "错误", 0x00000000);
                log_content = Encoding.UTF8.GetBytes($"{date} - ERROR: 账号或密码错误\n");
            } catch (Exception e) {
                MessageBox(0, e.Message, "错误", 0x00000000);
                log_content = Encoding.UTF8.GetBytes($"{date} - ERROR: {e.Message}\n");
            }
log_and_exit:
            log_fs.Write(log_content, 0, log_content.Length);
        } else {
            Application.Run(new MainForm(helper, check_interval));
        }
    }
}
