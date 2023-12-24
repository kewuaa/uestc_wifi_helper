{
    Copyright (c) 2020 by Ryan Joseph

    This unit implements a basic tokenizer
		
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
{$modeswitch typehelpers}

unit Scanner;
interface
uses
	SysUtils;

type
	EScanner = class(Exception);
	EScannerClass = class of EScanner;
	TScanner = class
    private type
      TFileInfo = record
        line: integer;
        column: integer;
      end;
    private
      currentIndex: integer;
      fileInfo: TFileInfo;
      consumedLineEnding: boolean;
    protected type
      {$scopedenums on}
      TIdentifier = Ansistring;
      TToken = (Unknown,
                // patterns
                ID, 
                Integer, 
                RealNumber,
                HexadecimalNumber,
                OctalNumber,
                BinaryNumber,
                // symbols
                DoubleQuote,
                SingleQuote,
                SquareBracketOpen,
                SquareBracketClosed,
                ParenthasisOpen,
                ParenthasisClosed,
                CurlyBracketOpen,
                CurlyBracketClosed,
                AngleBracketOpen,
                AngleBracketClosed,
                Equals,
                Colon,
                Comma,
                Semicolon,
                QuestionMark,
                ForwardSlash,
                BackSlash,
                ExclamationMark,
                Ampersand,
                Dash,
                Dot,
                Plus,
                EOL,
                EOF
                );
      {$scopedenums off}
    protected
      contents: ansistring;
      pattern: ansistring;
      token: TToken;
      c: char;
      readLineEndingsAsTokens: boolean;
    protected
      procedure Consume; inline;
      procedure Consume(t: TToken);
      procedure Consume(inChar: char);
      function TryConsume(t: TToken): boolean;
      function TryConsume(t: TToken; out s: shortstring): boolean; inline;
      function TryConsume(t: TToken; out s: ansistring): boolean; inline;

      function ReadToken: TToken;
      function ReadTo(count: integer): TToken;
      function ReadChar: char;
      function ReadWord: TIdentifier; virtual;
      function ReadNumber: string; virtual;
      function ReadString(count: integer): string;

      procedure ReadUntilEOL;
      procedure SkipSpace;

			function IsLineEnding: boolean;
			function IsWhiteSpace: boolean;

      procedure Advance(count: integer);
      procedure AdvancePattern(count: integer = 1);

     	function Peek(offset: integer = 1): char; overload;
      function Peek(str: string; offset: integer = 0): boolean; overload;
      function PeekString(count: integer): string;

      { Errors }
      procedure ParserError (messageString: string = '');
      function GetException: EScannerClass; virtual;

      { Handlers }
      procedure ParseToken; virtual;
      procedure UnknownCharacter(out cont: boolean); virtual;
		public
			constructor Create(str: ansistring);
			procedure Parse; virtual;
			destructor Destroy; override;
	end;

type
  TTokenMethods = type helper for TScanner.TToken
    function ToString: string;
  end;

implementation

{$macro on}
{$define TCharSetLineEnding:=#10, #12, #13}
{$define TCharSetWhiteSpace:=' ', '	'}
{$define TCharSetWord:='a'..'z','A'..'Z','_'}
{$define TCharSetInteger:='0'..'9'}
{$define TCharSetQuotes:='"', ''''}

function TTokenMethods.ToString: string;
begin
	case self of
		TScanner.TToken.ID:
			result := 'ID';
		TScanner.TToken.Integer:
			result := 'Integer';
		TScanner.TToken.RealNumber:
			result := 'Real';
		TScanner.TToken.DoubleQuote:
			result := '"';
		TScanner.TToken.SingleQuote:
			result := '''';
		TScanner.TToken.SquareBracketOpen:
			result := '[';
		TScanner.TToken.SquareBracketClosed:
			result := ']';
		TScanner.TToken.ParenthasisOpen:
			result := '(';
		TScanner.TToken.ParenthasisClosed:
			result := ')';
		TScanner.TToken.CurlyBracketOpen:
			result := '{';
		TScanner.TToken.CurlyBracketClosed:
			result := '}';
		TScanner.TToken.AngleBracketOpen:
			result := '<';
		TScanner.TToken.AngleBracketClosed:
			result := '>';
		TScanner.TToken.Equals:
			result := '=';
		TScanner.TToken.Colon:
			result := ':';
		TScanner.TToken.Comma:
			result := ',';
		TScanner.TToken.Semicolon:
			result := ';';
		TScanner.TToken.QuestionMark:
			result := '?';
		TScanner.TToken.ExclamationMark:
			result := '!';
		TScanner.TToken.ForwardSlash:
			result := '/';
		TScanner.TToken.BackSlash:
			result := '\';
		TScanner.TToken.Ampersand:
			result := '&';
		TScanner.TToken.Dash:
			result := '-';
		TScanner.TToken.Dot:
			result := '.';
		TScanner.TToken.Plus:
			result := '+';
		TScanner.TToken.EOF:
			result := 'End of File';
		TScanner.TToken.EOL:
			result := 'End of Line';
		otherwise
			raise exception.create('invalid token');
	end;
end;

procedure TScanner.Consume;
begin
	Consume(token);
end;

procedure TScanner.Consume(t: TToken);
begin
	if token = t then
	  ReadToken
	else
		ParserError('Got "'+token.ToString+'", expected "'+t.ToString+'"');
end;

procedure TScanner.Consume(inChar: char);
begin
	if c = inChar then
	  AdvancePattern
	else
		ParserError('Got "'+c+'", expected "'+inChar+'"');
end;

function TScanner.TryConsume(t: TToken; out s: shortstring): boolean; inline;
begin
	s := pattern;
	result := TryConsume(t);
end;

function TScanner.TryConsume(t: TToken; out s: ansistring): boolean;
begin
	s := pattern;
	result := TryConsume(t);
end;

function TScanner.TryConsume(t: TToken): boolean;
begin
	if token = t then
	  begin
	  	ReadToken;
	  	result := true;
	  end
	else
		result := false;
end;

procedure TScanner.ReadUntilEOL;
begin
	repeat
		ReadChar;
	until IsLineEnding;
end;

procedure TScanner.SkipSpace;
begin
	consumedLineEnding := false;
	while IsWhiteSpace do
		begin
			if IsLineEnding then
				consumedLineEnding := true;
			ReadChar;
		end;
end;

function TScanner.IsLineEnding: boolean;
begin
	result := c in [TCharSetLineEnding];
end;

function TScanner.IsWhiteSpace: boolean;
begin
	result := (c in [TCharSetWhiteSpace]) or IsLineEnding;
end;

function TScanner.Peek(offset: integer = 1): char;
begin
	if currentIndex + offset < length(contents) then
		result := contents[currentIndex + offset]
	else
		result := #0;
end;

function TScanner.Peek(str: string; offset: integer = 0): boolean;
var
	i, contentsOffset: integer;
begin
	result := false;
	for i := 0 to length(str) - 1 do
		begin
			contentsOffset := currentIndex + offset + i;
			if (contentsOffset < length(contents)) and (contents[contentsOffset] = str[i + 1]) then
				result := true
			else
				exit(false);
		end;
end;

function TScanner.PeekString(count: integer): string;
var
	i: integer;
begin
	result := '';
	for i := 0 to count - 1 do
		result += contents[currentIndex + i];
end;

function TScanner.ReadString(count: integer): string;
begin
	pattern := '';
	while count > 0 do
		begin
			pattern += c;
			ReadChar;
			Dec(count);
		end;
	result := pattern;
end;

function TScanner.ReadChar: char;
begin
	if IsLineEnding then
		begin
			fileInfo.line += 1;
			fileInfo.column := 0;
		end;
	currentIndex += 1;
	c := contents[currentIndex];
	fileInfo.column += 1;
	result := c;
end;

function TScanner.ReadWord: TIdentifier;
begin
	pattern := '';
	while c in [TCharSetWord, TCharSetInteger] do
		begin
			pattern += c;
			ReadChar;
		end;
	result := pattern;
end;


{ Appends character the current pattern and reads next character }
procedure TScanner.AdvancePattern(count: integer = 1);
begin
	while count > 0 do 
		begin
			pattern += c;
			ReadChar;
			Dec(count);
		end;
end;

{ Moves the scanner by 'count' characters }
procedure TScanner.Advance(count: integer);
begin
	while count > 0 do 
		begin
			ReadChar;
			Dec(count);
		end;
end;

{ Advances by "count" and reads token at new position }
function TScanner.ReadTo(count: integer): TToken;
begin
	Advance(count);
	result := ReadToken;
end;


function TScanner.ReadNumber: string;
begin
	pattern := '';
	token := TToken.Integer;

	if c = '-' then
		AdvancePattern;

	if c = '+' then
		AdvancePattern;

	while c in [TCharSetInteger, '.', 'e'] do
		begin
			// TODO: must be followed by a number!
			if c = 'e' then
				begin
					AdvancePattern;
					if c = '-' then
						begin
							AdvancePattern;
							while c in [TCharSetInteger] do
							  AdvancePattern;
							 break;
						end;
				end
			else if c = '.' then
				token := TToken.RealNumber;
			AdvancePattern;
		end;

	result := pattern;
end;

function TScanner.GetException: EScannerClass;
begin
	result := EScanner;
end;

procedure TScanner.ParserError(messageString: string = '');
begin
	// in case we pass in line ending from the current character "c"
	// replace these with something human readable
	messageString := StringReplace(messageString, #10, 'EOL', []);
	messageString := StringReplace(messageString, #12, 'EOL', []);
	messageString := StringReplace(messageString, #13, 'EOL', []);
	raise GetException.Create('Error at '+IntToStr(fileInfo.line)+':'+IntToStr(fileInfo.column)+': '+messageString);
end;

procedure TScanner.UnknownCharacter(out cont: boolean);
begin
	ParserError('unknown character "'+c+'"');	
end;

function TScanner.ReadToken: TToken;
label
	TokenRead;
var
  cont: boolean;
begin
	while currentIndex < length(contents) do 
		begin
			//writeln('  ', currentIndex, ':', c);
			case c of
			  '+':
			  	begin
			  		if Peek in [TCharSetInteger] then
			  			begin
			  				ReadNumber;
			  				goto TokenRead;
			  			end
			  		else
			  			begin
			  				token := TToken.Plus;
			  				ReadChar;
			  				goto TokenRead;
			  			end;
			  	end;
				'-':
					begin
						if Peek in [TCharSetInteger] then
							begin
								ReadNumber;
								goto TokenRead;
							end
						else
							begin
								token := TToken.Dash;
								ReadChar;
								goto TokenRead;
							end;
					end;
				TCharSetInteger:
					begin
						ReadNumber;
						goto TokenRead;
					end;
				TCharSetWord:
					begin
						token := TToken.ID;
						ReadWord;
						goto TokenRead;
					end;
				'[':
					begin
						token := TToken.SquareBracketOpen;
						ReadChar;
						goto TokenRead;
					end;
				']':
					begin
						token := TToken.SquareBracketClosed;
						ReadChar;
						goto TokenRead;
					end;
				'(':
					begin
						token := TToken.ParenthasisOpen;
						ReadChar;
						goto TokenRead;
					end;
				')':
					begin
						token := TToken.ParenthasisClosed;
						ReadChar;
						goto TokenRead;
					end;
				'{':
					begin
						token := TToken.CurlyBracketOpen;
						ReadChar;
						goto TokenRead;
					end;
				'}':
					begin
						token := TToken.CurlyBracketClosed;
						ReadChar;
						goto TokenRead;
					end;
				'<':
					begin
						token := TToken.AngleBracketOpen;
						ReadChar;
						goto TokenRead;
					end;
				'>':
					begin
						token := TToken.AngleBracketClosed;
						ReadChar;
						goto TokenRead;
					end;
				'=':
					begin
						token := TToken.Equals;
						ReadChar;
						goto TokenRead;
					end;
				':':
					begin
						token := TToken.Colon;
						ReadChar;
						goto TokenRead;
					end;
				',':
					begin
						token := TToken.Comma;
						ReadChar;
						goto TokenRead;
					end;
				';':
					begin
						token := TToken.Semicolon;
						ReadChar;
						goto TokenRead;
					end;
				'?':
					begin
						token := TToken.QuestionMark;
						ReadChar;
						goto TokenRead;
					end;
				'!':
					begin
						token := TToken.ExclamationMark;
						ReadChar;
						goto TokenRead;
					end;
				'.':
					begin
						token := TToken.Dot;
						ReadChar;
						goto TokenRead;
					end;
				'/':
				  begin
				  	token := TToken.ForwardSlash;
				  	ReadChar;
				  	goto TokenRead;
				  end;
				'\':
				  begin
				  	token := TToken.BackSlash;
				  	ReadChar;
				  	goto TokenRead;
				  end;
				TCharSetLineEnding,
				TCharSetWhiteSpace:
          begin
  					SkipSpace;
            if consumedLineEnding and readLineEndingsAsTokens then
              begin
                token := TToken.EOL;
                consumedLineEnding := false;
                goto TokenRead;
              end;
          end;
				otherwise
					begin
						cont := false;
            UnknownCharacter(cont);
            if cont then
            	continue;
            goto TokenRead;
          end;
			end;
		end;

	// if we got here we reached the end
	token := TToken.EOF;

	TokenRead:
		result := token;
end;

procedure TScanner.ParseToken;
begin
end;

procedure TScanner.Parse; 
begin
	ReadToken;
  while token <> TToken.EOF do
    ParseToken;
end;

constructor TScanner.Create(str: ansistring);
begin
	contents := str;
	contents += #0;
	currentIndex := 1;
	fileInfo.line := 1;
	fileInfo.column := 1;
	c := contents[currentIndex];
end;

destructor TScanner.Destroy;
begin
end;

end.