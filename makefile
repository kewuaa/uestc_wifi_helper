cli-linux64: ./uestc_wifi_helper_cli.pas ./uestc_wifi.pas
	fpc ./uestc_wifi_helper_cli.pas -FU./lib/Release/x86_64-linux -o./out/Release/uestc_wifi_helper_cli-x86_64-linux

cli-win64: ./uestc_wifi_helper_cli.pas ./uestc_wifi.pas
	fpc ./uestc_wifi_helper_cli.pas -FU./lib/Release/x86_64-win64 -o./out/Release/uestc_wifi_helper_cli-x86_64-win64.exe
