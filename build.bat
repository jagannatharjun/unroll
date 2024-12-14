SET SRC=E:\Projects\Gui\file-browser
SET QTDIR=C:\Qt\6.7.3\msvc2019_64
set LIBARCHIVE=C:\local\libarchive

call  "E:\Libraries\VisualStudio\IDE\Community\VC\Auxiliary\Build\vcvars64.bat" x64

echo ON

mkdir build
cd build
del CMakeCache.txt

cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo "-DCMAKE_PREFIX_PATH=%QTDIR%;%LIBARCHIVE%" %SRC%

ninja 

cd ..

mkdir release
cd release

copy ..\build\gui\file-browser.exe file-browser.exe 

%QTDIR%\bin\windeployqt.exe  --qmldir %SRC%\gui\qml file-browser.exe

copy %LIBARCHIVE%\bin\archive.dll archive.dll

cd ..

PAUSE
