; Rogue Syntax installer recipe: one script for every plugin (Inno Setup 6.3+).
; Saved as UTF-8 with a BOM, so Inno reads the ™ signs right. Keep it that way.
; Don't compile this file directly. Compile a plugin file from plugins\, which
; sets the defines below and includes this one. See README.md.
;
; What it does:
;   - shows eula.txt; the customer must accept it to go on
;   - copies the plugin's .vst3 into C:\Program Files\Common Files\VST3
;   - reads license.txt from the folder Setup was run from, and saves the
;     licence key to HKCU\Software\Rogue-Syntax\Registrations\<PluginKey>,
;     value License, as the logged-in user (not the elevated admin)
;   - no license.txt: asks the customer to paste the key (they may skip)
;   - an existing key is replaced only when a new one is given
; The installer checks only the key's shape. The plugin checks the signature.

#ifndef PluginName
  #error Compile a file in plugins\, not this one
#endif

#define Vst3Name ExtractFileName(Vst3Source)
; the name customers see, with the trademark sign
#define PluginTitle PluginName + "™"
#define RegPath "Software\Rogue-Syntax\Registrations\" + PluginKey

[Setup]
; {{ is a literal { in Inno, so this gives AppId={GUID}
AppId={{{#AppGuid}}
AppName={#PluginTitle}
AppVersion={#PluginVersion}
AppPublisher=Rogue Syntax Sound Design™ LLC
AppPublisherURL=https://rogue-syntax.com
; the customer must accept this before installing (paths are from plugins\)
LicenseFile=..\eula.txt
DefaultDirName={commonpf64}\Rogue Syntax\{#PluginKey}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\output
OutputBaseFilename={#PluginKey}Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#PluginTitle}
#ifdef SignTool
SignTool={#SignTool}
#endif

[Files]
#if DirExists(Vst3Source)
Source: "{#Vst3Source}\*"; DestDir: "{commoncf64}\VST3\{#Vst3Name}"; Flags: ignoreversion recursesubdirs createallsubdirs
#else
Source: "{#Vst3Source}"; DestDir: "{commoncf64}\VST3"; Flags: ignoreversion
#endif

[Code]
var
  KeyPage: TInputQueryWizardPage;
  FileToken: String;

function StripSpaces(S: String): String;
var
  i: Integer;
begin
  Result := '';
  for i := 1 to Length(S) do
    if (S[i] <> ' ') and (S[i] <> #9) and (S[i] <> #10) and (S[i] <> #13) then
      Result := Result + S[i];
end;

{ RS1.<base64url>.<base64url>: only these characters, so it's also safe on
  the reg.exe command line }
function LooksLikeToken(T: String): Boolean;
var
  i, dots: Integer;
  c: Char;
begin
  Result := False;
  if (Length(T) < 8) or (Length(T) > 2048) or (Copy(T, 1, 4) <> 'RS1.') then
    Exit;
  dots := 0;
  for i := 1 to Length(T) do begin
    c := T[i];
    if c = '.' then
      dots := dots + 1
    else if not (((c >= 'A') and (c <= 'Z')) or ((c >= 'a') and (c <= 'z')) or
                 ((c >= '0') and (c <= '9')) or (c = '-') or (c = '_')) then
      Exit;
  end;
  Result := (dots = 2) and (Pos('..', T) = 0) and (T[Length(T)] <> '.');
end;

{ the first line starting with RS1. }
function TokenFromFile(Path: String): String;
var
  Lines: TArrayOfString;
  i: Integer;
  L: String;
begin
  Result := '';
  if not LoadStringsFromFile(Path, Lines) then
    Exit;
  for i := 0 to GetArrayLength(Lines) - 1 do begin
    L := Trim(Lines[i]);
    { skip anything before RS1. on the line, e.g. a UTF-8 BOM }
    if Pos('RS1.', L) > 1 then
      L := Copy(L, Pos('RS1.', L), Length(L));
    if Copy(L, 1, 4) = 'RS1.' then begin
      if LooksLikeToken(L) then
        Result := L;
      Exit;
    end;
  end;
end;

procedure InitializeWizard;
begin
  FileToken := TokenFromFile(ExpandConstant('{src}\license.txt'));
  KeyPage := CreateInputQueryPage(wpWelcome, 'Licence key',
    'license.txt wasn''t found next to Setup.',
    'Paste your licence key (the line starting RS1. in your receipt email or ' +
    'license.txt), or leave it empty and click Next to skip. You can also ' +
    'paste it later in {#PluginTitle}: click Paste license.');
  KeyPage.Add('Licence key:', False);
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = KeyPage.ID) and (FileToken <> '');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  T: String;
begin
  Result := True;
  if CurPageID = KeyPage.ID then begin
    T := StripSpaces(KeyPage.Values[0]);
    if (T <> '') and not LooksLikeToken(T) then begin
      MsgBox('That doesn''t look like a licence key. It starts with RS1. ' +
        'Copy the whole line, or leave the box empty to skip.', mbError, MB_OK);
      Result := False;
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  T: String;
  Code: Integer;
begin
  if CurStep <> ssPostInstall then
    Exit;
  T := FileToken;
  if T = '' then
    T := StripSpaces(KeyPage.Values[0]);
  if T = '' then
    Exit;
  { reg.exe run as the original user, so the key lands in their HKCU even
    when Setup was elevated with another account }
  if not ExecAsOriginalUser(ExpandConstant('{sys}\reg.exe'),
       'add "HKCU\{#RegPath}" /v License /t REG_SZ /d "' + T + '" /f',
       '', SW_HIDE, ewWaitUntilTerminated, Code) or (Code <> 0) then
    MsgBox('{#PluginTitle} is installed, but the licence key couldn''t be ' +
      'saved. Open {#PluginTitle} and click Paste license.', mbError, MB_OK);
end;
