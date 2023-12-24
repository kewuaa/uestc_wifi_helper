program uestc_wifi_helper;

{$mode objfpc}{$H+}
// {$AppType CONSOLE}

uses
    {$IFDEF UNIX}
    cthreads,
    {$ENDIF}
    {$IFDEF HASAMIGA}
    athreads,
    {$ENDIF}
    Interfaces, // this includes the LCL widgetset
    Forms, ui, classes, sysutils,
    uestc_wifi, uestc_wifi_config
    { you can add units after this };

{$R *.res}
{$R template.rc}

var
    load_config_success: Boolean;
    log_file_path: String;
begin
    load_config_success := load_config();
    if not load_config_success then
    begin
        Exit();
    end;
    init_uestc_wifi(username, password, network_operator);
    if check_interval <= 0 then
    begin
        log_file_path := GetUserDir + 'uestc_wifi.log';
        check_once(log_file_path);
        Exit();
    end;
    RequireDerivedFormResource:=True;
    Application.Scaled:=True;
    Application.Initialize;
    Application.ShowMainForm:=False;
    Application.CreateForm(TForm1, Form1);
    Application.Run;
end.

