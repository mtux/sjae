; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "SJC"

; The file to write
OutFile "SJC_Setup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\SJE\SJC"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_SJC" "Install_Dir"

!define QTDIR	"c:\Qt\4.4.0"

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Microsoft Visual C Runtime"

  SetOutPath "$INSTDIR"

  File "vcredist_x86.exe"

  nsExec::ExecToStack 'vcredist_x86.exe'
  
  Delete "$INSTDIR\vcredist_x86.exe"
  
SectionEnd

Section "Qt 4.3.3 Runtime Libraries (MinGW, Commercial)"
  SetOutPath $INSTDIR  

  File "${QTDIR}\bin\QtCore4.dll"
  File "${QTDIR}\bin\QtGui4.dll"

  File "${QTDIR}\bin\QtNetwork4.dll"
  File "${QTDIR}\bin\QtXml4.dll"
  File "${QTDIR}\bin\QtScript4.dll"
 
SectionEnd

Section "SJC (required)"
  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR  
  ; Put files there
  File "..\bin\release\core.exe"

  ; Set output path to the plugins directory.
  SetOutPath $INSTDIR\plugins  
  File "..\bin\release\plugins\accounts.dll"
  File "..\bin\release\plugins\contact_list.dll"
  File "..\bin\release\plugins\icons.dll"
  File "..\bin\release\plugins\jabber.dll"
  File "..\bin\release\plugins\main_window.dll"
  File "..\bin\release\plugins\message_window.dll"
  File "..\bin\release\plugins\options.dll"
  File "..\bin\release\plugins\startup_status.dll"
  File "..\bin\release\plugins\status_bar.dll"
	
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_SJC "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SJC" "DisplayName" "SJC"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SJC" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SJC" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SJC" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

; Optional section (can be disabled by the user)
Section "Desktop Shortcut"

;create desktop shortcut
  CreateShortCut "$DESKTOP\SJC.lnk" "$INSTDIR\SJC.exe" ""

SectionEnd

Section "Start Menu Shortcuts"

; start menu shortcuts
  CreateDirectory "$SMPROGRAMS\SJE"
  CreateShortCut "$SMPROGRAMS\SJE\SJC.lnk" "$INSTDIR\core.exe" "" "$INSTDIR\core.exe" 0
  CreateShortCut "$SMPROGRAMS\SJE\Uninstall SJC.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SJC"
  DeleteRegKey HKLM SOFTWARE\NSIS_SJC

  ; Remove files and uninstaller
  Delete "$INSTDIR\plugins\*.*"
  Delete "$INSTDIR\*.*"

  ; Remove shortcuts, if any
  Delete "$DESKTOP\SJC.lnk"
  Delete "$SMPROGRAMS\SJE\SJC.lnk"
  Delete "$SMPROGRAMS\SJE\Uninstall SJC.lnk"
  RMDir "$SMPROGRAMS\SJE"

  ; Remove directories used
  RMDir "$INSTDIR\plugins"
  RMDir "$INSTDIR"
  RMDir "$PROGRAMFILES\SJE"
  
SectionEnd
