program uestc_wifi_helper_cli;

{$Mode objfpc} {$H+}

uses
    sysutils, uestc_wifi;

var
    network_operator: NetworkOperator;
begin
    if ParamCount < 3 then
    begin
        raise Exception.Create('Usage: uestc_wifi_helper_cli <username> <password> <network_operator>');
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
    check_once('');
end.
