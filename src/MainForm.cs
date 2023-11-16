using System;
using System.IO;
using System.Windows.Forms;
using System.Threading.Tasks;

class MainForm: Form {
    private UESTCWIFIHelper _wifi_helper;
    private int _interval;
    private Task _main_task;
    private NotifyIcon _notify_icon = new NotifyIcon();

    private async Task CheckOnce() {
        try {
            UESTCWIFIHelper.CheckedStatus status;
            try {
                status = await _wifi_helper.Check();
            } catch (UESTCWIFIHelper.NotConnectedException) {
                MessageBox.Show("未连接WiFi或网线", "提示");
                Exit(null, null);
                return;
            }
            switch (status) {
                case UESTCWIFIHelper.CheckedStatus.StillOnline:
                    break;
                case UESTCWIFIHelper.CheckedStatus.SuccessfullyLogin:
                    _notify_icon.ShowBalloonTip(1000, "INFO", "登录WiFi成功", ToolTipIcon.Info);
                    break;
                case UESTCWIFIHelper.CheckedStatus.DeviceWithinScope:
                    _notify_icon.ShowBalloonTip(1000, "INFO", "设备不在范围内", ToolTipIcon.Info);
                    await Task.Delay(3000);
                    Exit(null, null);
                    break;
            }
        } catch (UESTCWIFIHelper.IncorrectUsernameOrPasswordException) {
            MessageBox.Show("账号或密码错误", "错误");
            Exit(null, null);
        } catch (Exception e) {
            MessageBox.Show(e.Message, "错误");
            Exit(null, null);
        }
    }

    private async Task Run() {
        await Task.Delay(3000);
        while (true) {
            await CheckOnce();
            await Task.Delay(_interval);
        }
    }

    private async void CheckItemClick(object sender, EventArgs e) => await CheckOnce();

    private void InitNotifyIcon() {
        ContextMenuStrip menu = new ContextMenuStrip();
        menu.Items.Add("Check").Click += new EventHandler(CheckItemClick);
        menu.Items.Add("Exit").Click += new EventHandler(Exit);
        _notify_icon.Visible = true;
        _notify_icon.Icon = System.Drawing.Icon.ExtractAssociatedIcon(Path.Combine(Application.StartupPath, "UESTCWIFIHelper.exe"));
        _notify_icon.Text = "UESTC WiFi Helper";
        _notify_icon.ContextMenuStrip = menu;
        _notify_icon.ShowBalloonTip(1000, "INFO", "UESTC WiFi 助手已启动", ToolTipIcon.Info);
    }

    private void Exit(object sender, EventArgs e) {
        _notify_icon.ShowBalloonTip(1000, "INFO", "UESTC WiFi 助手已退出", ToolTipIcon.Info);
        _notify_icon.Visible = false;
        Application.Exit();
    }

    private void MainForm_Load(object sender, EventArgs e) {
        this.BeginInvoke(
            new Action(
                () => {
                    this.Visible = false;
                    this.Opacity = 1;
                }
            )
        );
    }

    public MainForm(UESTCWIFIHelper wifi_helper, int interval) {
        this.Opacity = 0;
        this.WindowState = FormWindowState.Minimized;
        this.ShowInTaskbar = false;
        this.Load += new EventHandler(MainForm_Load);
        InitNotifyIcon();

        _wifi_helper = wifi_helper;
        _interval = interval * 1000;
        _main_task = Run();
    }
}
