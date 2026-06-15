@echo off
REM ============================================================================
REM  BatchPasteTool - Build Script
REM  Supports: MSVC (Visual Studio) and MinGW-w64
REM ============================================================================
setlocal enabledelayedexpansion

echo.
echo ============================================
echo   BatchPasteTool Build Script
echo ============================================
echo.

REM ---- Check for MSVC (cl.exe) ----
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Using MSVC compiler (cl.exe)
    call :build_msvc
    goto :done
)

REM ---- Check for MinGW-w64 (g++) ----
where g++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [INFO] Using MinGW-w64 compiler (g++)
    call :build_mingw
    goto :done
)

echo [ERROR] No supported compiler found!
echo         Please install one of the following:
echo           - Visual Studio Build Tools (https://visualstudio.microsoft.com/downloads/)
echo           - MinGW-w64 (https://www.mingw-w64.org/)
echo.
echo         For MSVC: Run this script from "Developer Command Prompt for VS"
echo         For MinGW: Add g++.exe to your PATH
pause
exit /b 1

REM ========================
REM  MSVC Build
REM ========================
:build_msvc
echo [STEP] Compiling with MSVC...
echo.

REM Compile .rc to .res
rc.exe /nologo BatchPasteTool.rc
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Resource compilation failed!
    pause
    exit /b 1
)

REM Compile and link
cl.exe /nologo ^
    /O1 /Os /MT ^
    /EHsc ^
    /W3 ^
    /DUNICODE /D_UNICODE ^
    /Fe:BatchPasteTool.exe ^
    src\main.cpp ^
    BatchPasteTool.res ^
    /link ^
    gdiplus.lib ^
    comctl32.lib ^
    user32.lib ^
    gdi32.lib ^
    shell32.lib ^
    /SUBSYSTEM:WINDOWS ^
    /MACHINE:X64

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build complete: BatchPasteTool.exe
    echo [INFO]    File size:
    dir BatchPasteTool.exe | find "BatchPasteTool.exe"
) else (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
goto :eof

REM ========================
REM  MinGW-w64 Build
REM ========================
:build_mingw
echo [STEP] Compiling with MinGW-w64...
echo.

REM Compile .rc to .res (requires windres)
where windres.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    windres BatchPasteTool.rc -O coff BatchPasteTool.res
    set RESFILE=BatchPasteTool.res
) else (
    echo [WARN] windres not found, building without .rc resources
    set RESFILE=
)

g++.exe -std=c++17 ^
    -O2 -s ^
    -DUNICODE -D_UNICODE ^
    -o BatchPasteTool.exe ^
    src\main.cpp ^
    !RESFILE! ^
    -lgdiplus -lcomctl32 -luuid ^
    -static -mwindows -municode

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build complete: BatchPasteTool.exe
    echo [INFO]    File size:
    dir BatchPasteTool.exe | find "BatchPasteTool.exe"
) else (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
goto :eof

:done
echo.
echo ============================================
echo   Build finished
echo ============================================
echo.
echo   Run: BatchPasteTool.exe
echo.
pause
endlocal
