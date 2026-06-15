@echo off
REM ============================================================================
REM  BatchPasteTool - Publish Script (WPF .NET 8.0)
REM  Generates a self-contained single-file Windows x64 executable
REM ============================================================================

echo.
echo ============================================
echo   BatchPasteTool Publish (.NET 8.0 WPF)
echo ============================================
echo.

dotnet publish src\BatchPasteTool\BatchPasteTool.csproj ^
    -c Release ^
    -r win-x64 ^
    --self-contained true ^
    -p:PublishSingleFile=true ^
    -p:EnableCompressionInSingleFile=true ^
    -p:PublishTrimmed=false ^
    -o publish\win-x64\

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Published to publish\win-x64\BatchPasteTool.exe
    echo.
    dir publish\win-x64\BatchPasteTool.exe | find "BatchPasteTool.exe"
) else (
    echo [ERROR] Publish failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo   Publish finished
echo ============================================
echo.
pause
