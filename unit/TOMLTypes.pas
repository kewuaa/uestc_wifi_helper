{
    Copyright (c) 2020 by Ryan Joseph
  
    TOML Parser
    This unit implements the TOML data types

    ********************************************************************

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

}

{$mode objfpc}
{$modeswitch advancedrecords}
{$scopedenums on}

unit TOMLTypes;
interface
uses
  FGL, FPJSON, Classes, SysUtils;

type
  TTOMLStringType = AnsiString;
  TTOMLKeyType = ShortString;
  TTOMLFloat = Double;
  TTOMLValueType = Variant;
  TTOMLNumberType = (Integer, 
                     Float,
                     Octal,
                     Boolean,
                     Binary,
                     Hexadecimal);

  { TTOMLData }

  ETOMLData = class(Exception);
  TTOMLData = class
    private type
      TEnumerator = record
        private
          container: TTOMLData;
          currentValue: TTOMLData;
          index: integer;
        public
          constructor Create(inContainer: TTOMLData); 
          function MoveNext: Boolean;
          procedure Reset;
          property CurrentIndex: integer read index;
          property Current: TTOMLData read currentValue;
      end;
    private
      function GetItem(index: integer): TTOMLData; overload; virtual;
      function GetItem(key: TTOMLKeyType): TTOMLData; overload; virtual;
      procedure SetItem(index: integer; item: TTOMLData); virtual; overload;
      procedure SetItem(key: TTOMLKeyType; item: TTOMLData); virtual; overload; 
    public
      parent: TTOMLData;
    public
      function ToInteger: integer; virtual;
      function ToFloat: TTOMLFloat; virtual;
      function AsJSON: TJSONData; virtual;
      function Count: integer; virtual;
      property Items[index: integer]: TTOMLData read GetItem write SetItem; default;
      function GetEnumerator: TEnumerator;
  end;
  TTOMLDataList = specialize TFPGObjectList<TTOMLData>;
  TTOMLDataMap = specialize TFPGMapObject<String, TTOMLData>;
  TTOMLDataClass = class of TTOMLData;

  { TTOMLValue }

  TTOMLValue = class(TTOMLData)
    private
      m_value: TTOMLValueType;
    public
      constructor Create(const inValue: TTOMLValueType);
      function ToString: ansistring; override;
      function ToInteger: integer; override;
      function ToFloat: TTOMLFloat; override;
      function AsJSON: TJSONData; override;
      function TypeString: String;
      property Value: TTOMLValueType read m_value;
  end;

  { TTOMLNumber }

  TTOMLNumber = class(TTOMLValue)
    private
      m_type: TTOMLNumberType;
    public
      constructor Create(const inValue: TTOMLValueType; const inType: TTOMLNumberType);
      property &Type: TTOMLNumberType read m_type;
  end;

  { TTOMLDate }
  
  TTOMLDate = class(TTOMLData)
    public type
      TTime = record
        hours: integer;
        minutes: integer;
        seconds: double;
        { A suffix which, when applied to a time, denotes a UTC
        offset of 00:00; often spoken "Zulu" from the ICAO
        phonetic alphabet representation of the letter "Z". }
        z: boolean;
        function IsSet: boolean;
      end;
    public
      year: integer;
      month: integer;
      day: integer;
      time: TTime;
      
      { To unambiguously represent a specific instant in time, 
        you may use an RFC 3339 formatted date-time with offset. 
        https://tools.ietf.org/html/rfc3339}

      offset: TTime;
    public
      constructor Create(localTime: TTime); overload;

      function ToString: ansistring; override;
      function AsJSON: TJSONData; override;
      function ToISO8601String(roundSeconds: boolean = true): string;
      function AsDateTime: TDateTime;
  end;

  { TTOMLContainer }

  TTOMLContainer = class(TTOMLData);
  TTOMLContainerList = specialize TFPGList<TTOMLContainer>;

  { TTOMLArray }

  TTOMLArray = class(TTOMLContainer)
    private
      list: TTOMLDataList;
      function GetItem(index: integer): TTOMLData; override;
    public
      terminated: boolean;
    public
      constructor Create;
      destructor Destroy; override;

      procedure Add(const value: TTOMLValueType); overload;
      procedure Add(const data: TTOMLData); overload;
      
      function Last: TTOMLData;
      function AsJSON: TJSONData; override;
      function AsStrings: TStringList;
      function AsArray: TStringArray;
      function Count: integer; override;
  end;

  { TTOMLTable }

  TTOMLTable = class(TTOMLContainer)
    private
      map: TTOMLDataMap;
      m_name: string;
      function GetItem(key: TTOMLKeyType): TTOMLData; override;
      function GetItem(index: integer): TTOMLData; override;
      procedure SetItem(key: TTOMLKeyType; item: TTOMLData); override;
      function GetKey(index: integer): TTOMLKeyType;
    public
      defined: boolean;
      terminated: boolean;
      parentIsArray: boolean;
    public
      constructor Create(name: string = '');
      destructor Destroy; override;
      
      procedure Add(const key: TTOMLKeyType; const value: TTOMLValueType); overload;
      procedure Add(const key: TTOMLKeyType; const data: TTOMLData); overload;
      procedure Put(const key: String; const value: TTOMLValueType); overload;
      procedure Put(const key: String; const data: TTOMLData); overload;

      function Find(const key: TTOMLKeyType): TTOMLData;
      function Contains(const key: TTOMLKeyType; dataType: TTOMLDataClass = nil): boolean;
      function AsJSON: TJSONData; override;
      function Count: integer; override;

      property Name: string read m_name;
      property Keys[Index: Integer]: TTOMLKeyType read GetKey;
      property Values[Index: Integer]: TTOMLData read GetItem;
  end;

  { TTOMLDocument }

  TTOMLDocument = class(TTOMLTable);

implementation
uses
  Variants, Types, DateUtils;

{ TTOMLData }

function TTOMLData.GetEnumerator: TEnumerator;
begin
  result := TEnumerator.Create(self);
end;

constructor TTOMLData.TEnumerator.Create(inContainer: TTOMLData);
begin
  container := inContainer;
  index := 0;
end;
    
function TTOMLData.TEnumerator.MoveNext: Boolean;
var
  count: integer;
begin
  count := container.Count;
  if index = count then
    exit(false);
  while index < count do
    begin
      currentValue := container[index];
      index += 1;
      if currentValue <> Default(TTOMLData) then
        break;
    end;
  result := index <= count;
end;
    
procedure TTOMLData.TEnumerator.Reset;
begin
  index := 0;
end;

function TTOMLData.GetItem(index: integer): TTOMLData;
begin
  Assert(false, ClassName+' doesn''t implement indexing');
  result := nil;
end;

function TTOMLData.GetItem(key: TTOMLKeyType): TTOMLData;
begin
  Assert(false, ClassName+' doesn''t implement keys');
  result := nil;
end;

procedure TTOMLData.SetItem(index: integer; item: TTOMLData);
begin
  Assert(false, ClassName+' doesn''t implement setting by index');
end;

procedure TTOMLData.SetItem(key: TTOMLKeyType; item: TTOMLData);
begin
  Assert(false, ClassName+' doesn''t implement setting by key');
end;

function TTOMLData.Count: integer;
begin
  Assert(false, ClassName+' doesn''t implement indexing');
  result := 0;
end;

function TTOMLData.AsJSON: TJSONData;
begin
  Assert(false, 'TOML data can''t be converted to JSON');
  result := nil;
end;

function TTOMLData.ToInteger: integer;
begin
  Assert(false, 'TOML data can''t be converted to integer');
  result := 0;
end;

function TTOMLData.ToFloat: TTOMLFloat;
begin
  Assert(false, 'TOML data can''t be converted to float');
  result := 0;
end;

{ TTOMLValue }

function TTOMLValue.TypeString: String;
begin
  case VarType(value) of
    varEmpty: result := 'Empty';
    varNull: result := 'Null';
    varSingle: result := 'Single';
    varDouble: result := 'Double';
    varDecimal: result := 'Decimal';
    varCurrency: result := 'Currency';
    varDate: result := 'Date';
    varOleStr: result := 'UnicodeString';
    varString: result := 'Dynamic string';
    varBoolean: result := 'Boolean';
    varVariant: result := 'Variant';
    varUnknown: result := 'unknown';
    varShortInt: result := 'ShortInt';
    varSmallint: result := 'Smallint';
    varInteger: result := 'Integer';
    varInt64: result := 'Int64';
    varByte: result := 'Byte';
    varWord: result := 'Word'; 
    varLongWord: result := 'LongWord';
    varQWord: result := 'QWord';
    varError: result := 'ERROR';
    otherwise
      result := 'unknown';
  end;
end;

function TTOMLValue.AsJSON: TJSONData;
begin
  case VarType(value) of
    varSingle,
    varDouble,
    varDecimal,
    varCurrency:
      result := CreateJSON(Double(value));
    varDate:
      ;
    varOleStr,
    varStrArg,
    varString:
      result := CreateJSON(TTOMLStringType(value));
    varBoolean:
      result := CreateJSON(Boolean(value));
    varShortInt,
    varSmallint,
    varInteger,
    varInt64,
    varByte,
    varWord,
    varLongWord,
    varQWord:
      result := CreateJSON(LongInt(value));
    otherwise
      Assert(false, 'TOML value '+IntToStr(VarType(value))+' couldn''t be mapped to JSON value.');
    end;
end;

function TTOMLValue.ToInteger: integer;
begin
  result := integer(value);
end;

function TTOMLValue.ToFloat: TTOMLFloat;
begin
  result := TTOMLFloat(value);
end;

function TTOMLValue.ToString: ansistring;
begin
  result := ansistring(value);
end;

constructor TTOMLValue.Create(const inValue: TTOMLValueType);
begin
  m_value := inValue;
end;

{ TTOMLDate }

function TTOMLDate.TTime.IsSet: boolean;
begin
  result := (hours > 0) or (minutes > 0) or (seconds > 0);
end;

constructor TTOMLDate.Create(localTime: TTime);
begin
  time := localTime;
end;

function TTOMLDate.ToString: ansistring;
begin
  result := ToISO8601String(false);
end;

function TTOMLDate.ToISO8601String(roundSeconds: boolean): string;
var
  s: string;
begin

  result := Format('%.*d',[4, year])+'-'+
            Format('%.*d',[2, month])+'-'+
            Format('%.*d',[2, day]);

  if time.IsSet then
    begin
      result += 'T';
      result += Format('%.*d',[2, time.hours])+':'+
                Format('%.*d',[2, time.minutes])+':';
                
      if roundSeconds then
        result += Format('%.*d',[2, Trunc(time.seconds)])
      else
        begin
          s := FloatToStr(time.seconds);
          if length(s) = 1 then
            result += '0';
          result += s;
        end;

      if time.Z then
        result += 'Z';

      if offset.IsSet then
        begin
          result += '-';
          result += Format('%.*d',[2, time.hours])+':'+
                    Format('%.*d',[2, time.minutes]);
        end;
    end;
end;

function TTOMLDate.AsJSON: TJSONData;
begin
  result := CreateJSON(ToString);
end;

function TTOMLDate.AsDateTime: TDateTime;
begin
  result := ISO8601ToDate(ToISO8601String);
end;

{ TTOMLNumber }

constructor TTOMLNumber.Create(const inValue: TTOMLValueType; const inType: TTOMLNumberType);
begin
  m_value := inValue;
  m_type := inType;
end;

{ TTOMLArray }

function TTOMLArray.GetItem(index: integer): TTOMLData;
begin
  result := list[index];
end;

function TTOMLArray.AsStrings: TStringList;
var
  data: TTOMLData;
begin
  result := TStringList.Create;
  for data in list do
    result.Add(AnsiString(data));
end;

function TTOMLArray.AsArray: TStringArray;
var
  i: integer;
begin
  result := nil;
  SetLength(result, Count);
  for i := 0 to Count - 1 do
    result[i] := AnsiString(list[i]);
end;

function TTOMLArray.AsJSON: TJSONData;
var
  arr: TJSONArray;
  data: TTOMLData;
begin
  arr := TJSONArray.Create;
  for data in list do
    arr.Add(data.AsJSON);
  result := arr;
end;

function TTOMLArray.Last: TTOMLData;
begin
  result := list.Last;
end;

function TTOMLArray.Count: integer;
begin
  result := list.Count;
end;

procedure TTOMLArray.Add(const value: TTOMLValueType);
begin
  Add(TTOMLValue.Create(value));
end;

procedure TTOMLArray.Add(const data: TTOMLData);
begin
  data.parent := self;
  list.Add(data);
end;

constructor TTOMLArray.Create;
begin
  list := TTOMLDataList.Create(true);
end;

destructor TTOMLArray.Destroy;
begin
  list.Free;
  inherited;
end;

{ TTOMLTable }

function TTOMLTable.GetKey(index: integer): TTOMLKeyType;
begin
  result := map.Keys[index];
end;

function TTOMLTable.GetItem(key: TTOMLKeyType): TTOMLData;
var
  data: TTOMLData;
begin
  if map.TryGetData(key, data) then
    result := data
  else
    raise ETOMLData.Create('Key "'+key+'" doesn''t exist in table "'+name+'"');
end;

function TTOMLTable.GetItem(index: integer): TTOMLData;
begin
  result := map.data[index];
end;

procedure TTOMLTable.SetItem(key: TTOMLKeyType; item: TTOMLData);
begin
  Put(key, item);
end;

function TTOMLTable.Count: integer;
begin
  result := map.Count;
end;

function TTOMLTable.AsJSON: TJSONData;
var
  i: integer;
  obj: TJSONObject;
begin
  obj := TJSONObject.Create;
  for i := 0 to map.Count - 1 do
    obj.Add(map.Keys[i], map.Data[i].AsJSON);
  result := obj;
end;

procedure TTOMLTable.Add(const key: String; const data: TTOMLData);
begin
  if Contains(key) then
    raise ETOMLData.Create('Key "'+key+'" already exists in table "'+name+'"');
  data.parent := self;
  map.Add(key, data);
end;

procedure TTOMLTable.Add(const key: String; const value: TTOMLValueType);
begin
  Add(key, TTOMLValue.Create(value));
end;

procedure TTOMLTable.Put(const key: String; const value: TTOMLValueType);
begin
  Put(key, TTOMLValue.Create(value));
end;

procedure TTOMLTable.Put(const key: String; const data: TTOMLData);
var
  index: integer;
begin
  data.parent := self;

  if map.Find(key, index) then
    begin
      // replace existing item
      map.data[index] := data;
    end
  else
    map.Add(key, data);
end;

function TTOMLTable.Contains(const key: TTOMLKeyType; dataType: TTOMLDataClass = nil): boolean;
var
  data: TTOMLData;
begin
  result := map.TryGetData(key, data);
  if result and 
    assigned(dataType) and 
    not data.InheritsFrom(dataType) then
    result := false;
end;

function TTOMLTable.Find(const key: TTOMLKeyType): TTOMLData;
var
  data: TTOMLData;
begin
  if map.TryGetData(key, data) then
    result := data
  else
    result := nil;
end;

constructor TTOMLTable.Create(name: string);
begin
  m_name := name;
  defined := false;
  map := TTOMLDataMap.Create(true);
  map.Sorted := true;
end;

destructor TTOMLTable.Destroy;
begin
  map.Free;
  inherited;
end;


end.