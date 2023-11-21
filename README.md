# 介绍

一个用于登录泥电校园网的脚本，支持寝室的电信和移动WiFi，以及教研室网络。

# 下载

可在[发布页面](https://github.com/kewuaa/uestc_wifi_helper/releases)下载可执行文件。（版本需要高于Windows 10 1903）

# 配置

首次运行会提示需要配置，配置文件默认如下：

``` toml
username = "your username"

password = "your password"

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
```

配置保存在`%USERPROFILE%/uestc_wifi.toml`

日志保存在`%USERPROFILE%/uestc_wifi.log`

# 使用

`check_interval`为正数时，将运行一个后台程序，间隔`check_interval`秒检查一次网络连接情况

`check_interval`为非正数时，静默检查一次网络连接情况，结果保存至日志文件中

`win+r`打开Windows命令运行框，并输入`shell:startup`，打开开机自启动目录，放入可执行文件的快捷方式，进而实现开机启动后台程序

![](./snapshots/snapshot1.png)

也可使用Windows任务计划程序来进行更精细化的设置。
