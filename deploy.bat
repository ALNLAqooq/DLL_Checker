@echo off
echo ========================================
echo DLL Dependency Checker - Deployment Script
echo ========================================
echo.

REM Check if windeployqt is available
where windeployqt >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: windeployqt not found in PATH
    echo Please make sure Qt bin directory is in your PATH
    echo Example: C:\Qt\6.5.0\msvc2019_64\bin
    pause
    exit /b 1
)

REM Check if Release directory exists
if not exist "Release\DLLChecker.exe" (
    echo ERROR: Release\DLLChecker.exe not found
    echo Please build the project in Release mode first
    pause
    exit /b 1
)

echo Deploying Qt dependencies...
windeployqt Release\DLLChecker.exe --release --no-translations

echo.
echo ========================================
echo Deployment completed successfully!
echo ========================================
echo.
echo The Release directory now contains all necessary files.
echo You can copy the entire Release folder to the target machine.
echo.
pause
