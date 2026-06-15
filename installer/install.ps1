# BatchPasteTool Installer
# Installs to %LocalAppData%\Programs\BatchPasteTool

$ErrorActionPreference = "Stop"

$AppName = "BatchPasteTool"
$InstallDir = "$env:LOCALAPPDATA\Programs\$AppName"
$SourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ExePath = "$InstallDir\BatchPasteTool.exe"

Write-Host "Installing $AppName to $InstallDir..."

# Create install directory
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}

# Copy all files
Write-Host "Copying files..."
Copy-Item -Path "$SourceDir\*" -Destination $InstallDir -Recurse -Force -Exclude "install.ps1"

$WshShell = New-Object -ComObject WScript.Shell
$StartMenu = [Environment]::GetFolderPath("Programs")
$Desktop = [Environment]::GetFolderPath("Desktop")

$ShortcutPath = "$StartMenu\$AppName.lnk"
$DesktopShortcut = "$Desktop\$AppName.lnk"
$UninstallerPath = "$InstallDir\uninstall.bat"
$UninstallShortcutPath = "$StartMenu\Uninstall $AppName.lnk"

# Create uninstaller script FIRST (so shortcuts can reference it)
$UninstallerContent = @'
@echo off
echo Uninstalling BatchPasteTool...
del /f /q "{APPPATH}" 2>nul
del /f /q "{UNINST_LNK}" 2>nul
del /f /q "{DESKTOP_LNK}" 2>nul
rmdir /s /q "{INSTALLDIR}" 2>nul
echo BatchPasteTool has been uninstalled.
pause
'@
$UninstallerContent = $UninstallerContent.Replace('{APPPATH}', $ShortcutPath)
$UninstallerContent = $UninstallerContent.Replace('{UNINST_LNK}', $UninstallShortcutPath)
$UninstallerContent = $UninstallerContent.Replace('{DESKTOP_LNK}', $DesktopShortcut)
$UninstallerContent = $UninstallerContent.Replace('{INSTALLDIR}', $InstallDir)
$UninstallerContent | Out-File -FilePath $UninstallerPath -Encoding ASCII

# Create Start Menu shortcut
Write-Host "Creating Start Menu shortcut..."
$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = $ExePath
$Shortcut.WorkingDirectory = $InstallDir
$Shortcut.Description = "Batch text paste utility"
$Shortcut.Save()

# Create Desktop shortcut
Write-Host "Creating Desktop shortcut..."
$Shortcut2 = $WshShell.CreateShortcut($DesktopShortcut)
$Shortcut2.TargetPath = $ExePath
$Shortcut2.WorkingDirectory = $InstallDir
$Shortcut2.Description = "Batch text paste utility"
$Shortcut2.Save()

# Create Uninstall shortcut in Start Menu
Write-Host "Creating Uninstall shortcut..."
$Shortcut3 = $WshShell.CreateShortcut($UninstallShortcutPath)
$Shortcut3.TargetPath = $UninstallerPath
$Shortcut3.WorkingDirectory = $InstallDir
$Shortcut3.Description = "Uninstall BatchPasteTool"
$Shortcut3.Save()

Write-Host ""
Write-Host "============================================"
Write-Host "  $AppName installed successfully!"
Write-Host "  Location: $InstallDir"
Write-Host "  Uninstall: Start Menu -> Uninstall $AppName"
Write-Host "============================================"

# Launch the application
Start-Process $ExePath
