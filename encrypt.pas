unit encrypt;

{$mode objfpc}{$H+}

interface

function xencode(const str: String; const key: String): String;

implementation

uses
    sysutils, classes, math;

type
    UInt32Array = array of UInt32;
    WCharArray = array of Word;

procedure s(const a: String; b: Boolean; out res: UInt32Array);
var
    i: Integer = 0;
    j: Integer;
    x: UInt32;
    c: Integer;
begin
    c := Length(a);
    if b then
    begin
        SetLength(res, (c - 1) div 4 + 2);
        res[Length(res) - 1] := c;
    end
    else
    begin
        SetLength(res, (c - 1) div 4 + 1);
    end;
    while i < c do
    begin
        x := UInt32(a[i + 1]);
        for j := 1 to 3 do
        begin
            if i + j < c then
            begin
                x := x or UInt32(a[i + j + 1]) << (j * 8);
            end
            else
            begin
                Break;
            end;
        end;
        res[i div 4] := x;
        i := i + 4;
    end;
end;

// s := '{"username":"202321011111@dx","password":"hzy..123456","ip":"10.20.15.20","acid":"3","enc_ver":"srun_bx1"}'
// key := '4407928503af441b473f1e3cadd053214fce3ea0ce2ef529700adced1d84b9dd'
// 'cOM7jHUi9cVk2sWckU+2bIvJ9PL4tLKOIsOoNjrZL5oqXbG/so6+CEoSLpvzw4+0cEmmneu+hlrQWCCn+bfYgwOeb1voXqxV1r28fYJ19xGvuTMBYMYSJGkJNqb/dZLClR7VE7tl3YjKLmJhz8SZ++=='
function xencode(const str: String; const key: String): String;
const
    PAD = '=';
var
    ALPHA: String = 'LVoJPiCN2R8G90yg+hmFHuacZ1OWMnrsSTXkYpUq/3dlbfKwv6xztjI7DeBE45QA';
    i: Integer;
    v, k: UInt32Array;
    n: Integer;
    z, y, c, m, e, p, d: UInt32;
    q: Integer;
    len: Integer;
    str2: WCharArray;
    b10: UInt32;
    imax: Integer;
    b: String;
begin
    if str = '' then
    begin
        raise Exception.Create('str is empty');
    end;
    s(str, True, v);
    s(key, False, k);
    n := Length(v) - 1;
    z := v[n];
    y := v[0];
    c := UInt32(-1640531527);
    q := Floor(6 + 52.0 / Length(v));
    d := 0;
    while (q > 0) do
    begin
        q := q - 1;
        d := d + c and (-1);
        e := d >> 2 and 3;
        for p := 0 to n - 1 do
        begin
            y := v[p + 1];
            m := z >> 5 xor y << 2;
            m := m + ((y >> 3 xor z << 4) xor (d xor y));
            m := m + ((k[(p and 3) xor e] xor z));
            v[p] := v[p] + m and (-1);
            z := v[p];
        end;
        y := v[0];
        m := z >> 5 xor y << 2;
        m := m + ((y >> 3 xor z << 4) xor (d xor y));
        m := m + ((k[(n and 3) xor e] xor z));
        v[n] := v[n] + m and (-1);
        z := v[n];
    end;
    len := Length(v) * 4;
    SetLength(str2, len);
    for i := 0 to Length(v) - 1 do
    begin
        str2[i * 4] := Word(v[i] and $FF);
        str2[i * 4 + 1] := Word(v[i] >> 8 and $FF);
        str2[i * 4 + 2] := Word(v[i] >> 16 and $FF);
        str2[i * 4 + 3] := Word(v[i] >> 24 and $FF);
    end;


    // base64 encode part
    i := 0;
    b10 := 0;
    imax := len - len mod 3;
    b := '';
    while i < imax do
    begin
        b10 := (str2[i] << 16) or (str2[i + 1] << 8) or str2[i + 2];
        b := b
            + ALPHA[b10 >> 18 + 1]
            + ALPHA[(b10 >> 12) and 63 + 1]
            + ALPHA[(b10 >> 6) and 63 + 1]
            + ALPHA[b10 and 63 + 1];
        i := i + 3;
    end;
    i := imax;
    case len - imax of
        1:
        begin
            b10 := str2[i] << 16;
            b := b
                + ALPHA[b10 >> 18 + 1]
                + ALPHA[(b10 >> 12) and 63 + 1]
                + PAD
                + PAD;
        end;
        2:
        begin
            b10 := (str2[i] << 16) or (str2[i + 1] << 8);
            b := b
                + ALPHA[b10 >> 18 + 1]
                + ALPHA[(b10 >> 12) and 63 + 1]
                + ALPHA[(b10 >> 6) and 63 + 1]
                + PAD;
        end;
    end;
    Result := b;
end;

end.
