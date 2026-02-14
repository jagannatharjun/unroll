@echo off
SETLOCAL EnableDelayedExpansion

:: --- Configuration ---
SET "SRC=E:\Projects\Gui\file-browser"
SET "QTDIR=C:\Qt\6.10.1\msvc2022_64"
SET "LIBARCHIVE=C:\local\libarchive"
SET "VCVARS=E:\Libraries\VisualStudio\IDE\Community\VC\Auxiliary\Build\vcvars64.bat"

echo [1/6] Validating paths...
if not exist "%SRC%" echo Error: Source path not found. && pause && exit /b 1
if not exist "%VCVARS%" echo Error: Visual Studio vcvars not found. && pause && exit /b 1

:: --- Environment Setup ---
echo [2/6] Initializing MSVC Environment...
call "%VCVARS%" x64
if %ERRORLEVEL% neq 0 echo Failed to set up MSVC environment. && pause && exit /b 1

:: --- Build Process ---
if not exist build mkdir build
cd build || exit /b 1

echo [3/6] Running CMake...
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDB_TEST=OFF "-DCMAKE_PREFIX_PATH=%QTDIR%;%LIBARCHIVE%" "%SRC%"
if %ERRORLEVEL% neq 0 echo CMake configuration failed. && pause && exit /b 1

echo [4/6] Running Ninja Build...
ninja
if %ERRORLEVEL% neq 0 echo Build failed. && pause && exit /b 1

cd ..

:: --- Deployment ---
echo [5/6] Preparing Release Folder...
if not exist release mkdir release
cd release || exit /b 1

copy /Y ..\build\gui\file-browser.exe file-browser.exe || goto :error
copy /Y ..\build\gui\file-browser.pdb file-browser.pdb || goto :error

echo [6/6] Running windeployqt...
"%QTDIR%\bin\windeployqt.exe" --qmldir "%SRC%\gui\qml" file-browser.exe
if %ERRORLEVEL% neq 0 echo Deployment tool failed. && pause && exit /b 1

copy /Y "%LIBARCHIVE%\bin\archive.dll" archive.dll || goto :error

echo.
echo ===========================================
echo Build and Deployment Successful!
echo ===========================================
cd ..
pause
exit /b 0

:error
echo.
echo !------- ERROR DETECTED -------!
echo An error occurred during the file copying process.
echo !------------------------------!
pause
exit /b 1