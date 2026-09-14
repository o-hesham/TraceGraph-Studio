#ifndef AppSourceDir
  #error AppSourceDir must be supplied by the build script
#endif
#ifndef AppOutputDir
  #error AppOutputDir must be supplied by the build script
#endif
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

#define AppName "TraceGraph Studio"
#define AppPublisher "TraceGraph Studio"
#define AppExeName "tracegraph-studio.exe"

[Setup]
AppId={{E647781D-2B4B-4B79-93E8-EF1FB15D4BC7}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={localappdata}\Programs\TraceGraph Studio
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir={#AppOutputDir}
OutputBaseFilename=TraceGraph-Studio-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#AppExeName}
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "{#AppSourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent
