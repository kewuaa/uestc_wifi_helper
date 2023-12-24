unit ui;

{$mode objfpc}{$H+}

interface

uses
  Classes, SysUtils, Forms, Controls, Graphics, Dialogs, ExtCtrls,
  Menus, uestc_wifi, uestc_wifi_config;

type

  { TForm1 }

  TForm1 = class(TForm)
      MenuItem1: TMenuItem;
      MenuItem2: TMenuItem;
    PopupMenu1: TPopupMenu;
    Timer1: TTimer;
    TrayIcon1: TTrayIcon;
    procedure FormCreate(Sender: TObject);
    procedure MenuItem1Click(Sender: TObject);
    procedure MenuItem2Click(Sender: TObject);
    procedure Timer1StartTimer(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
  private
    procedure check(verbose: Boolean);
  public

  end;

var
  Form1: TForm1;

implementation

{$R *.lfm}

{ TForm1 }

procedure TForm1.check(verbose: Boolean);
begin
    try
        TrayIcon1.BalloonTitle := 'INFO';
        case wifi.check() of
            StillOnline:
            begin
                if verbose then
                begin
                  TrayIcon1.BalloonHint := '设备已在线';
                  TrayIcon1.ShowBalloonHint();
                end;
            end;
            SuccessfullyLogin:
            begin
                TrayIcon1.BalloonHint := '登陆成功';
                TrayIcon1.ShowBalloonHint();
            end;
            DeviceWithinScope:
            begin
                TrayIcon1.BalloonHint := '设备不在范围内，程序退出';
                TrayIcon1.ShowBalloonHint();
                Close();
            end;
        end;
    except
        on E: NotConnectedException do
        begin
            TrayIcon1.BalloonTitle := 'WARNING';
            TrayIcon1.BalloonHint := '未连接到网络，请检查网络连接后重启程序';
            TrayIcon1.ShowBalloonHint();
            Close();
        end;
        on E: IncorrectUsernameOrPasswordException do
        begin
            ShowMessage('用户名或密码错误，请检查配置文件后重启程序');
            Close();
        end;
        on E: Exception do
        begin
            ShowMessage('程序因未知错误退出: ' + E.Message);
            Close();
        end;
    end;
end;

procedure TForm1.Timer1StartTimer(Sender: TObject);
begin
    Timer1.interval := check_interval * 1000;
end;

procedure TForm1.MenuItem1Click(Sender: TObject);
begin
    Close;
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
    Hide();
end;

procedure TForm1.MenuItem2Click(Sender: TObject);
begin
    check(True);
end;

procedure TForm1.Timer1Timer(Sender: TObject);
begin
    check(False);
end;

end.

