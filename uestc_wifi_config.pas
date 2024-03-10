unit uestc_wifi_config;

interface

uses
    classes, sysutils, process, dialogs, lcltype, TOML, uestc_wifi;

function load_config(): Boolean;

var
    username: string;
    password: string;
    network_operator: NetworkOperator;
    check_interval: integer;

implementation

function load_config(): Boolean;
var
    config_file_path: String;
    config_template: TResourceStream;
    config: TStringStream;
    {$IfDef WINDOWS}
    process: TProcess;
    {$EndIf}
    toml_doc: TTOMLDocument;
begin
    config_file_path := GetUserDir + 'uestc_wifi.toml';
    config := TStringStream.Create;
    if not FileExists(config_file_path) then
    begin
        ShowMessage('你需要在 ' + config_file_path + ' 中保存你的用户名和密码等配置');
        config_template := TResourceStream.Create(HINSTANCE, 'CONFIG_TEMPLATE', RT_RCDATA);
        config.CopyFrom(config_template, config_template.Size);
        config.SaveToFile(config_file_path);
        config.Free();
        config_template.Free();

        {$IfDef WINDOWS}
        process := TProcess.Create(Nil);
        process.Executable := 'notepad';
        process.Parameters.Add(config_file_path);
        process.Execute();
        process.Free();
        {$EndIf}
        Exit(False);
    end;
    config.LoadFromFile(config_file_path);
    Result := True;
    toml_doc := GetTOML(config.DataString);
    if toml_doc.Contains('username') then
    begin
        username := toml_doc['username'].ToString();
    end
    else
    begin
        ShowMessage('未在 ' + config_file_path + ' 中读取到 username');
        Exit(false);
    end;
    if toml_doc.Contains('password') then
    begin
        password := toml_doc['password'].ToString();
    end
    else
    begin
        ShowMessage('未在 ' + config_file_path + ' 中读取到 password');
        Exit(false);
    end;
    if toml_doc.Contains('network_operator') then
    begin
        case toml_doc['network_operator'].ToInteger() of
            0: network_operator := CTCC;
            1: network_operator := CMCC;
            2: network_operator := UESTC;
            3: network_operator := CTCC_UESTC;
        else
            raise Exception.Create('未知的 network_operator');
        end;
    end
    else
    begin
        network_operator := CTCC;
    end;
    if toml_doc.Contains('check_interval') then
    begin
        check_interval := toml_doc['check_interval'].ToInteger();
    end
    else
    begin
        check_interval := 30;
    end;
    toml_doc.Free();
end;

end.
