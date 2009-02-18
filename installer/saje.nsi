; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "Saje"

; The file to write
OutFile "saje_setup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Saje"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\NSIS_SAJE" "Install_Dir"

!define QTDIR	$%QTDIR%

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

Section "Qt Runtime Libraries"
  SetOutPath $INSTDIR  

  File "${QTDIR}\bin\QtCore4.dll"
  File "${QTDIR}\bin\QtGui4.dll"

  File "${QTDIR}\bin\QtNetwork4.dll"
  File "${QTDIR}\bin\QtXml4.dll"
  File "${QTDIR}\bin\QtScript4.dll"
  File "${QTDIR}\bin\QtWebKit4.dll"
 
SectionEnd

Section "Qt Plugins"
  SetOutPath $INSTDIR  

  File "${QTDIR}\plugins\accessible\qtaccessiblewidgets4.dll"
  File "${QTDIR}\plugins\imageformats\qjpeg4.dll"
  File "${QTDIR}\plugins\sqldrivers\qsqlite4.dll"
 
SectionEnd

Section "Saje (required)"
  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR  
  ; Put files there
  File "..\bin\release\core.exe"

  ; Set output path to the plugins directory.
  SetOutPath $INSTDIR\plugins  
  File "..\bin\release\plugins\accounts.dll"
  File "..\bin\release\plugins\add_contact.dll"
  File "..\bin\release\plugins\autoaway.dll"
  File "..\bin\release\plugins\autoreconnect.dll"
  File "..\bin\release\plugins\contact_list.dll"
  File "..\bin\release\plugins\contactinfo.dll"
  File "..\bin\release\plugins\events.dll"
  File "..\bin\release\plugins\history.dll"
  File "..\bin\release\plugins\icons.dll"
  File "..\bin\release\plugins\jabber.dll"
  File "..\bin\release\plugins\jabber.dll"
  File "..\bin\release\plugins\log_window.dll"
  File "..\bin\release\plugins\main_window.dll"
  File "..\bin\release\plugins\messagenotify.dll"
  File "..\bin\release\plugins\message_window.dll"
  File "..\bin\release\plugins\options.dll"
  File "..\bin\release\plugins\popup.dll"
  File "..\bin\release\plugins\startup_status.dll"
  File "..\bin\release\plugins\status_bar.dll"
  File "..\bin\release\plugins\styles.dll"
	
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_SAJE "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SAJE" "DisplayName" "SAJE"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SAJE" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SAJE" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SAJE" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
SectionEnd

; Optional section (can be disabled by the user)
Section "Desktop Shortcut"

;create desktop shortcut
  CreateShortCut "$DESKTOP\Saje.lnk" "$INSTDIR\core.exe" ""

SectionEnd

Section "Start Menu Shortcuts"

; start menu shortcuts
  CreateDirectory "$SMPROGRAMS\Saje"
  CreateShortCut "$SMPROGRAMS\Saje\Saje.lnk" "$INSTDIR\core.exe" "" "$INSTDIR\core.exe" 0
  CreateShortCut "$SMPROGRAMS\Saje\Uninstall Saje.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SAJE"
  DeleteRegKey HKLM SOFTWARE\NSIS_SAJE

  ; Remove files and uninstaller
  Delete "$INSTDIR\plugins\*.*"
  Delete "$INSTDIR\*.*"

  ; Remove shortcuts, if any
  Delete "$DESKTOP\saje.lnk"
  Delete "$SMPROGRAMS\Saje\Saje.lnk"
  Delete "$SMPROGRAMS\Saje\Uninstall Saje.lnk"
  RMDir "$SMPROGRAMS\Saje"

  ; Remove directories used
  RMDir "$INSTDIR\plugins"
  RMDir "$INSTDIR"
  RMDir "$PROGRAMFILES\Saje"
  
SectionEnd
