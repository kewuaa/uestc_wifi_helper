# 介绍

一个用于登录泥电校园网的脚本，支持寝室的电信和移动WiFi，以及教研室网络。

支持登陆的网址:
* 10.253.0.235
* 10.253.0.237

# 下载

可在[发布页面](https://github.com/kewuaa/uestc_wifi_helper/releases)下载可执行文件，分为纯命令行程序和托盘程序。

linux下的托盘程序依赖qt6pas

# 配置

首次运行会提示需要配置，[默认配置文件](./template.toml)

配置和日志均保存在用户目录下，`uestc_wifi.toml` 和 `uestc_wifi.log`

Windows 和 Linux 下的用户目录分别为 `C:\Users\用户名` 和 `/home/用户名`

# 使用

## 托盘程序

`check_interval`为正数时，将运行一个后台程序，间隔`check_interval`秒检查一次网络连接情况

`check_interval`为非正数时，静默检查一次网络连接情况，结果保存至日志文件中

可自行设置开机自启动或其他方式。

## 命令行程序

```
uestc_wifi_helper_cli <username> <password> <network_operator> [-l]
```

前三个参数和配置文件中含义一致，不加`-l`会将结果输出至标准输出，加`-l`会输出至日志文件中。
