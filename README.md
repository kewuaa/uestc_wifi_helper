# 介绍

一个用于登录泥电校园网的脚本，支持寝室的电信和移动WiFi，以及教研室网络。

支持登陆的网址:
* 10.253.0.235
* 10.253.0.237

# 下载

可在[发布页面](https://github.com/kewuaa/uestc_wifi_helper/releases)下载可执行文件，分为纯命令行程序和托盘程序。

依赖openssl，windows下可在[此处](https://indy.fulgan.com/SSL/)下载，linux一般自带openssl，但是托盘程序依赖qt5pas

windows也可以安装[以前的版本](https://github.com/kewuaa/uestc_wifi_helper/releases/tag/v0.3.1)

# 配置

首次运行会提示需要配置，[默认配置文件](./template.toml)

配置和日志均保存在用户目录下，`uestc_wifi.toml` 和 `uestc_wifi.log`

# 使用

`check_interval`为正数时，将运行一个后台程序，间隔`check_interval`秒检查一次网络连接情况

`check_interval`为非正数时，静默检查一次网络连接情况，结果保存至日志文件中

可自行设置开机自启动或其他方式。
