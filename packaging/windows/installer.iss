#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif

#define MyAppName "SUB ORBIT"
#define MyAppPublisher "Echoes Lab"
#define MyAppURL "https://github.com/echoeslabmusicsoftware/Sub-Orbit"

[Setup]
AppId={{E4C3B2A1-7F6D-4E5A-8B9C-0D1E2F3A4B5C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={cf}\VST3
OutputBaseFilename=SubOrbit-Windows-Installer
OutputDir=Output
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
DisableDirPage=yes
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
UninstallDisplayName={#MyAppName}
WizardStyle=modern
PrivilegesRequired=admin

[Files]
Source: "..\..\build\SubOrbit_artefacts\Release\VST3\SUB ORBIT.vst3\*"; DestDir: "{cf}\VST3\SUB ORBIT.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{cf}\VST3\SUB ORBIT.vst3"
