@echo off
setlocal

echo ===================================================
echo   SuperVideoCutter C++ Portable Build Automation
echo ===================================================

:: Setup paths
set MSBUILD_PATH="C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
set SOLUTION_DIR=%~dp0
set PORTABLE_DIR=%SOLUTION_DIR%..\PortableApp

echo Solution Directory: %SOLUTION_DIR%
echo Target Portable Directory: %PORTABLE_DIR%

:: Ensure MSBuild exists
if not exist %MSBUILD_PATH% (
    echo [ERROR] MSBuild.exe was not found at %MSBUILD_PATH%.
    echo Please make sure Visual Studio Community 18 is installed.
    exit /b 1
)

:: Create PortableApp folder if not exists
if not exist "%PORTABLE_DIR%" (
    mkdir "%PORTABLE_DIR%"
)

:: echo.
:: echo [INFO] Pre-downloading and packaging dependencies for embedded resources...
:: powershell -NoProfile -ExecutionPolicy Bypass -File "%SOLUTION_DIR%bundle_dependencies.ps1"
:: if errorlevel 1 goto ERROR_BUNDLE

:: ---------------------------------------------------
:: 1. Compile x64 Release Binary
:: ---------------------------------------------------
echo.
echo [INFO] Compiling x64 (64-bit) Release Binary...
%MSBUILD_PATH% "%SOLUTION_DIR%SuperVideoCutter.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto ERROR_X64

:: ---------------------------------------------------
:: 2. Compile x86 (Win32) Release Binary
:: ---------------------------------------------------
echo.
echo [INFO] Compiling x86 (32-bit) Release Binary...
%MSBUILD_PATH% "%SOLUTION_DIR%SuperVideoCutter.sln" /t:Rebuild /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 goto ERROR_X86

:: ---------------------------------------------------
:: 3. Export and Package
:: ---------------------------------------------------
echo.
echo [INFO] Exporting and packaging compiled portable executables...

:: Copy x64 single portable binary to root
copy /y "%SOLUTION_DIR%bin\x64\Release\SuperVideoCutter.exe" "%PORTABLE_DIR%\SuperVideoCutter_x64.exe"
if not exist "%PORTABLE_DIR%\SuperVideoCutter_x64.exe" goto ERROR_COPY

:: Also copy to the structured subfolder as fallback
copy /y "%SOLUTION_DIR%bin\x64\Release\SuperVideoCutter.exe" "%PORTABLE_DIR%\SuperVideoCutter_x64\SuperVideoCutter.exe"

echo [SUCCESS] Exported: PortableApp\SuperVideoCutter_x64.exe (Single-file Portable App!)

:: Copy x86 single portable binary to root
copy /y "%SOLUTION_DIR%bin\Win32\Release\SuperVideoCutter.exe" "%PORTABLE_DIR%\SuperVideoCutter_x86.exe"
if not exist "%PORTABLE_DIR%\SuperVideoCutter_x86.exe" goto ERROR_COPY

:: Also copy to the structured subfolder as fallback
copy /y "%SOLUTION_DIR%bin\Win32\Release\SuperVideoCutter.exe" "%PORTABLE_DIR%\SuperVideoCutter_x86\SuperVideoCutter.exe"

echo [SUCCESS] Exported: PortableApp\SuperVideoCutter_x86.exe (Single-file Portable App!)

echo.
echo ===================================================
echo   PORTABLE BUILDS COMPLETED SUCCESSFULLY!
echo ===================================================
exit /b 0

:ERROR_X64
echo [ERROR] Failed to compile x64 Release Binary!
exit /b 1

:ERROR_X86
echo [ERROR] Failed to compile x86 (Win32) Release Binary!
exit /b 1

:ERROR_BUNDLE
echo [ERROR] Failed to download/bundle dependencies!
exit /b 1

:ERROR_COPY
echo [ERROR] Failed to copy compiled binaries to PortableApp!
exit /b 1
