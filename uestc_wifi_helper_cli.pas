program uestc_wifi_helper_cli;

{$Mode objfpc} {$H+}

uses
    sysutils, uestc_wifi;

var
    network_operator: NetworkOperator;
    log_file_path: String = '';
begin
    if (ParamCount > 0) and (ParamStr(1) = '-h') then
    begin
        WriteLn('Usage: uestc_wifi_helper_cli <username> <password> <network_operator> [-l]');
        Exit();
    end;
    if ParamCount < 3 then
    begin
        raise Exception.Create('Invalid usage, use -h for help');
    end;
    case StrToInt(ParamStr(3)) of
        0: network_operator := CTCC;
        1: network_operator := CMCC;
        2: network_operator := UESTC;
        3: network_operator := CTCC_UESTC;
        else
            raise Exception.Create('Invalid network operator');
    end;
    init_uestc_wifi(ParamStr(1), ParamStr(2), network_operator);
    if ParamCount > 3 then
    begin
        if ParamStr(4) <> '-l' then
        begin
            raise Exception.Create('Invalid usage, use -h for help');
        end;
        log_file_path := GetUserDir() + 'uestc_wifi.log';
    end;
    check_once(log_file_path);
end.
