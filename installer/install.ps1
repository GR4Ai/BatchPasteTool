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

# Create Start Menu shortcut
$StartMenu = [Environment]::GetFolderPath("Programs")
$ShortcutPath = "$StartMenu\$AppName.lnk"
Write-Host "Creating Start Menu shortcut..."
$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = $ExePath
$Shortcut.WorkingDirectory = $InstallDir
$Shortcut.Description = "Batch text paste utility"
$Shortcut.Save()

# Create Desktop shortcut
$Desktop = [Environment]::GetFolderPath("Desktop")
$DesktopShortcut = "$Desktop\$AppName.lnk"
Write-Host "Creating Desktop shortcut..."
$Shortcut2 = $WshShell.CreateShortcut($DesktopShortcut)
$Shortcut2.TargetPath = $ExePath
$Shortcut2.WorkingDirectory = $InstallDir
$Shortcut2.Description = "Batch text paste utility"
$Shortcut2.Save()

# Create uninstaller script
$Uninstaller = @"
@echo off
echo Uninstalling $AppName...
del /q "$ShortcutPath" 2>nul
del /q "$DesktopShortcut" 2>nul
rmdir /s /q "$InstallDir" 2>nul
echo $AppName has been uninstalled.
pause
"@
$UninstallerPath = "$InstallDir\uninstall.bat"
$Uninstaller | Out-File -FilePath $UninstallerPath -Encoding ASCII

Write-Host ""
Write-Host "============================================"
Write-Host "  $AppName installed successfully!"
Write-Host "  Location: $InstallDir"
Write-Host "  Uninstall: $UninstallerPath"
Write-Host "============================================"

# Launch the application
Start-Process $ExePath
