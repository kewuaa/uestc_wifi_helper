{
    Copyright (c) 2020 by Ryan Joseph

    Main unit for fpTOML parser
    
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
{$unitpath sources}

unit TOML;
interface
uses
  TOMLParser,
  TOMLTypes;

type
  TTOMLStringType = TOMLTypes.TTOMLStringType;
  TTOMLKeyType = TOMLTypes.TTOMLStringType;
  TTOMLValueType = TOMLTypes.TTOMLStringType;
  TTOMLNumberType = TOMLTypes.TTOMLStringType;

type
  TTOMLData = TOMLTypes.TTOMLData;
  TTOMLValue = TOMLTypes.TTOMLValue;
  TTOMLNumber = TOMLTypes.TTOMLNumber;
  TTOMLDate = TOMLTypes.TTOMLDate;
  TTOMLArray = TOMLTypes.TTOMLArray;
  TTOMLTable = TOMLTypes.TTOMLTable;
  TTOMLDocument = TOMLTypes.TTOMLDocument;

function GetTOML(contents: TTOMLStringType): TTOMLDocument;

{ TOMLData Operators }

operator Explicit (right: TTOMLData): ansistring; overload;
operator Explicit (right: TTOMLData): shortstring; overload;
operator Explicit (right: TTOMLData): integer; overload;
operator Explicit (right: TTOMLData): single; overload;
operator Explicit (right: TTOMLData): double; overload;

operator := (right: variant): TTOMLData;

implementation

{ TOMLData Operators }

operator := (right: variant): TTOMLData;
begin
  result := TTOMLValue.Create(right);
end;

operator Explicit (right: TTOMLData): ansistring;
begin
  result := right.ToString;
end;

operator Explicit (right: TTOMLData): shortstring;
begin
  result := right.ToString;
end;

operator Explicit (right: TTOMLData): integer;
begin
  result := right.ToInteger;
end;

operator Explicit (right: TTOMLData): single;
begin
  result := right.ToFloat;
end;

operator Explicit (right: TTOMLData): double;
begin
  result := right.ToFloat;
end;

function GetTOML(contents: TTOMLStringType): TTOMLDocument;
begin
  result := TOMLParser.GetTOML(contents);
end;

end.