# 介绍

一个用于登录泥电校园网的脚本，支持寝室的电信和移动WiFi，以及教研室网络。

支持登陆的网址:
* 10.253.0.235
* 10.253.0.237

# 下载

可在[发布页面](https://github.com/kewuaa/uestc_wifi_helper/releases)下载可执行文件，支持windows和linux。

有两个程序`uestc_wifi`和`uestc_wifi_helper`。

# 编译

使用cmake对项目进行构建，通过CPM管理依赖，windows下建议使用msvc进行编译。

```
cmake -B build -S .
cmake --build build
```

# 配置

首次运行会提示需要配置，[默认配置文件](./template.toml)

配置和日志均保存在用户目录下，`uestc_wifi.toml` 和 `uestc_wifi.log`

Windows 和 Linux 下的用户目录分别为 `C:\Users\用户名` 和 `/home/用户名`

# 使用

## uestc_wifi

```
uestc_wifi -h
```

`uestc_wifi`为命令行程序，可以用于简单的一次检查用户是否在线，不在线则进行登陆。

## uestc_wifi_helper

```
uestc_wifi_helper -h
```

`uestc_wifi_helper`在linux为一个后台服务，通过dbus和NetworkManager通信，监听网络状态变更事件，网络连接断开时会停止检测。

在windows下为一个托盘程序，通过Network List Manager监听网络变更事件，同样在网络断开连接时会停止检测。

都会在后台间隔`check_interval`秒检查一次网络连接情况，检查到用户下线时，将会使用配置文件中的信息进行登陆。

可自行设置开机自启动或其他方式。
