; Setup for a ReXGlue PC port (The Legend of Spyro: Dawn of the Dragon).
; Adapted from the Legend of Spyro: A New Beginning port's Setup.
;
; Installs the programs per user (no administrator rights; the settings file
; beside them stays writable), then copies the game from the player's own
; copy: an Xbox 360 disc image or a folder with the extracted files. The copy
; is done by the launcher (<game>_launcher.exe /import), which reads the disc
; image and checks the game's version; Setup only asks where it is.
;
; Built by package.ps1, which passes:
;   GameId, AppTitle, AppVersion, AppGuid, StageDir, OutDir, IconFile, DiscText, GameBytes

[Setup]
AppId={{{#AppGuid}}
AppName={#AppTitle} (PC port)
AppVersion={#AppVersion}
AppVerName={#AppTitle} (PC port) {#AppVersion}
DefaultDirName={autopf}\{#AppTitle}
DefaultGroupName={#AppTitle}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
ExtraDiskSpaceRequired={#GameBytes}
OutputDir={#OutDir}
OutputBaseFilename={#GameId}-PC-{#AppVersion}-Setup
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\{#GameId}_launcher.exe
UninstallDisplayName={#AppTitle} (PC port)
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#AppTitle}"; Filename: "{app}\{#GameId}_launcher.exe"
Name: "{group}\Play (skip launcher)"; Filename: "{app}\{#GameId}.exe"; WorkingDir: "{app}"
Name: "{group}\Read me"; Filename: "{app}\README.txt"
Name: "{autodesktop}\{#AppTitle}"; Filename: "{app}\{#GameId}_launcher.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\{#GameId}_launcher.exe"; Description: "Open the launcher"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; The copied game and what the programs write beside themselves. Saves are
; elsewhere (Documents\{#GameId}) and stay.
Type: filesandordirs; Name: "{app}\game"
Type: filesandordirs; Name: "{app}\logs"
Type: files; Name: "{app}\{#GameId}.toml"
Type: files; Name: "{app}\{#GameId}.log"

[Code]
var
  SourcePage: TInputOptionWizardPage;
  ImagePage: TInputFileWizardPage;
  FolderPage: TInputDirWizardPage;
  SourceChosen: Boolean;

procedure InitializeWizard;
begin
  SourcePage := CreateInputOptionPage(wpSelectDir,
    'Game files', 'Where is your copy of the game?',
    'This port contains none of the game. Its files ({#DiscText}) are copied from your own copy.',
    True, False);
  SourcePage.Add('From an Xbox 360 disc image (.iso, full or trimmed)');
  SourcePage.Add('From a folder with the game''s extracted files (default.xex at the top)');
  SourcePage.Add('Later, from the launcher (or keep the game files already installed)');
  SourcePage.SelectedValueIndex := 0;

  ImagePage := CreateInputFilePage(SourcePage.ID,
    'Game files', 'Choose the disc image', 'An image of the game''s Xbox 360 disc.');
  ImagePage.Add('Disc image:', 'Xbox 360 disc images (*.iso;*.xiso)|*.iso;*.xiso|All files|*.*', '.iso');

  FolderPage := CreateInputDirPage(ImagePage.ID,
    'Game files', 'Choose the game''s folder', 'The folder with default.xex in it.', False, '');
  FolderPage.Add('');
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  { Reinstalling over a copy that has the game already: keep it. }
  if (CurPageID = SourcePage.ID) and not SourceChosen then begin
    SourceChosen := True;
    if FileExists(ExpandConstant('{app}\game\default.xex')) then
      SourcePage.SelectedValueIndex := 2;
  end;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
  { /GAMESRC or a silent install: no questions about the game files. }
  if (PageID = SourcePage.ID) or (PageID = FolderPage.ID) or (PageID = ImagePage.ID) then
    if WizardSilent or (ExpandConstant('{param:GAMESRC|}') <> '') then begin
      Result := True;
      Exit;
    end;
  if PageID = ImagePage.ID then
    Result := SourcePage.SelectedValueIndex <> 0;
  if PageID = FolderPage.ID then
    Result := SourcePage.SelectedValueIndex <> 1;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = FolderPage.ID) and not FileExists(AddBackslash(FolderPage.Values[0]) + 'default.xex') then begin
    MsgBox('There is no default.xex there. Choose the top folder of the game.', mbError, MB_OK);
    Result := False;
  end;
  if (CurPageID = ImagePage.ID) and not FileExists(ImagePage.Values[0]) then begin
    MsgBox('Choose the disc image file.', mbError, MB_OK);
    Result := False;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Src: String;
  Code: Integer;
begin
  if CurStep <> ssPostInstall then
    Exit;
  { /GAMESRC="image or folder" picks the source without the pages, for a
    silent install; a silent install without it leaves the game for later. }
  Src := ExpandConstant('{param:GAMESRC|}');
  if Src = '' then begin
    if WizardSilent then
      Exit;
    case SourcePage.SelectedValueIndex of
      0: Src := ImagePage.Values[0];
      1: Src := FolderPage.Values[0];
    else
      Exit;
    end;
  end;
  { A trailing backslash would escape the closing quote ("E:\"). }
  if Copy(Src, Length(Src), 1) = '\' then
    Src := Src + '\';
  WizardForm.StatusLabel.Caption := 'Copying the game files from your copy of the game...';
  if not Exec(ExpandConstant('{app}\{#GameId}_launcher.exe'), '/import "' + Src + '"', ExpandConstant('{app}'),
              SW_SHOW, ewWaitUntilTerminated, Code) or (Code <> 0) then
    MsgBox('The game files were not installed. You can install them later with "Install game files..." ' +
           'in the launcher.', mbInformation, MB_OK);
end;
