unit uestc_wifi;

{$IfDef WINDOWS}
{$Codepage UTF8}
{$EndIf}
{$Mode objfpc}{$H+}
{$Macro on}

interface

uses
    classes, sysutils,
    fphttpclient, opensslsockets,
    fpjson, jsonparser
    {$IfDef Lazarus}
    , dialogs
    {$Define warning := ShowMessage}
    {$Else}
    {$Define warning := WriteLn}
    {$EndIf}
    ;

type
    NotConnectedException = class(Exception);

    DeviceWithinScopeException = class(Exception);

    IncorrectUsernameOrPasswordException = class(Exception);

    NetworkOperator = (
        CTCC,
        CMCC,
        UESTC,
        CTCC_UESTC
    );

    CheckedStatus = (
        StillOnline,
        DeviceWithinScope,
        SuccessfullyLogin
    );

    UESTCWiFi = class
    private
        _username: String;
        _password: String;
        _ac_id: Integer;
        _target_ip: String;
        _network_operator: String;
        _client: TFPHTTPClient;

        function _parse_response(const res: String): TJSONObject;
        function _check_connect(): Boolean;
        procedure _check_online(out online: Boolean; out online_ip: String);
        procedure _login(const client_ip: String);
    public
        constructor Create(const username: String; const password: String; const network_operator: NetworkOperator);
        destructor Destroy; override;
        function check(): CheckedStatus;
    end;

var
    wifi: UESTCWiFi;

procedure init_uestc_wifi(const username: String; const password: String; const network_operator: NetworkOperator);
procedure check_once(log_file_path: String);

implementation

uses
    dateutils, hmac, sha1, httpprotocol, encrypt;

const
    CALLBACK_STRING = 'jQuery112409729861590799633_1698107269291'; // length = 41

function get_timestamp(): int64;
begin
    Result := MilliSecondsBetween(Now, EncodeDateTime(1970, 1, 1, 0, 0, 0, 0));
end;

function UESTCWiFi._parse_response(const res: String): TJSONObject;
var
    json_str: String;
begin
    if not res.StartsWith(CALLBACK_STRING) then
    begin
        raise Exception.Create('Unknown response');
    end;
    json_str := res.Substring(42, res.Length - 43);
    Result := GetJSON(json_str) as TJSONObject;
end;

function UESTCWiFi._check_connect(): Boolean;
begin
    try
        _client.Get('http://' + _target_ip);
    except
       on E: Exception do
        begin
            Exit(
                (_client.ResponseStatusCode > 0) and (_client.ResponseStatusCode < 400)
            );
        end;
    end;
    Result := _client.ResponseStatusCode < 400;
end;

procedure UESTCWiFi._check_online(out online: Boolean; out online_ip: String);
var
    timestamp: int64;
    res: String;
    json: TJSONObject;
begin
    timestamp := get_timestamp();
    res := _client.Get(
        'http://' + _target_ip + '/cgi-bin/rad_user_info'
        + '?callback=' + CALLBACK_STRING
        + '&_=' + IntToStr(timestamp)
    );
    json := _parse_response(res);
    case json.Get('error', '') of
        'ok':
        begin
            online := True;
            online_ip := json.Get('online_ip', '');
        end;
        'not_online_error':
        begin
            online := False;
            online_ip := json.Get('client_ip', '');
        end;
        'speed_limit_error':
        begin
            json.Free();
            raise Exception.Create('Authentication requests are too frequent');
        end;
        else
        begin
            json.Free();
            raise Exception.Create('Got unexpected message from response when checking online status');
        end;
    end;
    json.Free();
end;

procedure UESTCWiFi._login(const client_ip: String);
var
    username: String;
    data: TJSONObject;
    res: String;
    json: TJSONObject;
    json_str: String;
    token: String;
    encoded_str: String;
    info: String;
    password_md5: String;
    chksum: String;
    error: String;
    error_msg: String;
begin
    username := _username + '@' + _network_operator;
    res := _client.Get(
        'http://' + _target_ip + '/cgi-bin/get_challenge'
        + '?callback=' + CALLBACK_STRING
        + '&username=' + username
        + '&ip=' + client_ip
        + '&_=' + IntToStr(get_timestamp())
    );
    json := _parse_response(res);
    if json.Get('error', '') <> 'ok' then
    begin
        raise Exception.Create('Fetch challenge failed: ' + json.Get('error_msg', ''));
    end;
    token := json.Get('challenge', '');
    json.Free();
    Assert(token <> '', 'Challenge is empty');
    data := TJSONObject.Create();
    data.Add('username', username);
    data.Add('password', _password);
    data.Add('ip', client_ip);
    data.Add('acid', _ac_id);
    data.Add('enc_ver', 'srun_bx1');
    json_str := data.AsJSON;
    encoded_str := xencode(json_str, token);
    info := '{SRBX1}' + encoded_str;
    password_md5 := HMACMD5(token, _password);
    chksum := SHA1Print(SHA1String(
        token
        + username
        + token
        + password_md5
        + token
        + IntToStr(_ac_id)
        + token
        + client_ip
        + token
        + '200'
        + token
        + '1'
        + token
        + info
    ));
    res := _client.Get(
        'http://' + _target_ip + '/cgi-bin/srun_portal'
        + '?callback=' + CALLBACK_STRING
        + '&action=login'
        + '&username=' + username
        + '&password=%7BMD5%7D' + password_md5
        + '&ac_id=' + IntToStr(_ac_id)
        + '&ip=' + client_ip
        + '&chksum=' + chksum
        + '&info=' + HTTPEncode(info)
        + '&n=200'
        + '&type=1'
        + '&os=Windows%2010'
        + '&name=Windows'
        + '&double_stack=0'
        + '&_=' + IntToStr(get_timestamp())
    );
    json := _parse_response(res);
    error := json.Get('error', '');
    if error <> 'ok' then
    begin
        error_msg := json.Get('error_msg', '');
        json.Free();
        case error_msg of
            'INFO Error锛宔rr_code=2': raise DeviceWithinScopeException.Create('The device is not within the scope of certification');
            'E2901: (Third party 1)bind_user2: ldap_bind error',
            'E2901: (Third party 1)ldap_first_entry error': raise IncorrectUsernameOrPasswordException.Create('Incorrect username or password');
            else raise Exception.Create('Login failed: error: ' + error + ', error_msg: ' + error_msg);
        end;
    end;
    json.Free();
end;

constructor UESTCWiFi.Create(const username: String; const password: String; const network_operator: NetworkOperator);
begin
    _username := username;
    _password := password;
    case network_operator of
        CTCC:
        begin
            _ac_id := 3;
            _target_ip := '10.253.0.235';
            _network_operator := 'dx';
        end;
        CMCC:
        begin
            _ac_id := 3;
            _target_ip := '10.253.0.235';
            _network_operator := 'cmcc';
        end;
        UESTC:
        begin
            _ac_id := 1;
            _target_ip := '10.253.0.237';
            _network_operator := 'dx-uestc';
        end;
        CTCC_UESTC:
        begin
            _ac_id := 1;
            _target_ip := '10.253.0.237';
            _network_operator := 'dx';
        end;
        else raise Exception.Create('Unknown network operator');
    end;
    _client := TFPHTTPClient.Create(Nil);
    _client.AllowRedirect := True;
    _client.AddHeader('Content-Type', 'application/x-www-form-urlencoded');
    _client.AddHeader('User-Agent', 'Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/102.0.0.0 Safari/537.36');
    _client.AddHeader('Host', _target_ip);
    _client.Cookies.Add('lang=zh-CN');
end;

destructor UESTCWiFi.Destroy;
begin
    _client.Free();
    inherited Destroy();
end;

function UESTCWiFi.check(): CheckedStatus;
var
    online: Boolean;
    online_ip: String;
begin
    if not _check_connect() then
    begin
        raise NotConnectedException.Create('Not connected to network cable or WiFi');
    end;
    _check_online(online, online_ip);
    if online then
    begin
        Exit(StillOnline);
    end;
    try
        _login(online_ip);
        Exit(SuccessfullyLogin);
    except
        on e: DeviceWithinScopeException do
        begin
            Exit(DeviceWithinScope);
        end;
    end;
end;

procedure init_uestc_wifi(const username: String; const password: String; const network_operator: NetworkOperator);
begin
    wifi := UESTCWiFi.Create(username, password, network_operator);
end;

procedure check_once(log_file_path: String);
var
    time: String;
    msg: UnicodeString;
    log_file: TextFile;
begin
    time := DateTimeToStr(Now);
    try
        case wifi.check() of
            StillOnline: msg := time + ' - INFO: 设备已在线';
            DeviceWithinScope: msg := time + ' - INFO: 设备不在范围内';
            SuccessfullyLogin: msg := time + ' - INFO: 登录成功';
        end;
    except
        on E: NotConnectedException do
        begin
            warning('未连接到网络');
            msg := time + ' - ERROR: 未连接到网络';
        end;
        on E: IncorrectUsernameOrPasswordException do
        begin
            warning('用户名或密码错误');
            msg := time + ' - ERROR: 用户名或密码错误';
        end;
        on E: Exception do
        begin
            warning('未知错误: ' + E.Message);
            msg := time + ' - ERROR: 未知错误: ' + E.Message;
        end;
    end;
    if log_file_path <> '' then
    begin
        AssignFile(log_file, log_file_path);
        if FileExists(log_file_path) then
        begin
            Append(log_file);
        end
        else
        begin
            ReWrite(log_file);
        end;
        WriteLn(log_file, msg);
        CloseFile(log_file);
    end
    else
    begin
        WriteLn(msg);
    end;
    Exit();
end;

end.
