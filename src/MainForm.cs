namespace UESTCWIFIHelper;

class MainForm: Form {
    private Kewuaa.UESTCWIFIHelper _wifi_helper;
    private int _interval;
    private Task _main_task;
    private NotifyIcon _notify_icon = new();

    private async Task CheckOnce() {
        try {
            var status = await _wifi_helper.Check();
            var msg = status switch {
                Kewuaa.UESTCWIFIHelper.CheckedStatus.SuccessfullyLogin => "登陆WiFi成功",
                    Kewuaa.UESTCWIFIHelper.CheckedStatus.DeviceWithinScope => "设备不在范围内",
                    Kewuaa.UESTCWIFIHelper.CheckedStatus.NotConnected => "未连接WiFi或网线",
                    _ => null,
            };
            if (msg is not null) {
                _notify_icon.ShowBalloonTip(1000, "INFO", msg, ToolTipIcon.Info);
            }
        } catch (Exception e) {
            MessageBox.Show(e.ToString(), "错误");
        }
    }

    private async Task Run() {
        while (true) {
            await CheckOnce();
            await Task.Delay(_interval);
        }
    }

    private async void CheckItemClick(object? sender, EventArgs e) => await CheckOnce();

    private void InitNotifyIcon() {
        ContextMenuStrip menu = new();
        menu.Items.Add("Check").Click += new(CheckItemClick);
        menu.Items.Add("Exit").Click += new(Exit);
        _notify_icon.Visible = true;
        _notify_icon.Icon = Icon.ExtractAssociatedIcon(Path.Combine(Application.StartupPath, "UESTCWIFIHelper.exe"));
        _notify_icon.Text = "UESTC WiFi Helper";
        _notify_icon.ContextMenuStrip = menu;
        _notify_icon.ShowBalloonTip(1000, "INFO", "UESTC WiFi 助手已启动", ToolTipIcon.Info);
    }

    private void Exit(object? sender, EventArgs e) {
        _notify_icon.Visible = false;
        Application.Exit();
    }

    public MainForm(Kewuaa.UESTCWIFIHelper wifi_helper, int interval) {
        this.WindowState = FormWindowState.Minimized;
        this.ShowInTaskbar = false;
        InitNotifyIcon();

        _wifi_helper = wifi_helper;
        _interval = interval * 60 * 60 * 1000;
        _main_task = Run();
    }
}
