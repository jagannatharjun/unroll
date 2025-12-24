SET SRC=E:\Projects\Gui\file-browser
SET QTDIR=C:\Qt\6.10.1\msvc2022_64
set LIBARCHIVE=C:\local\libarchive

call  "E:\Libraries\VisualStudio\IDE\Community\VC\Auxiliary\Build\vcvars64.bat" x64

echo ON

mkdir build
cd build
:: del CMakeCache.txt

cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo  -DDB_TEST=OFF "-DCMAKE_PREFIX_PATH=%QTDIR%;%LIBARCHIVE%" %SRC%

ninja 

cd ..

mkdir release
cd release

copy ..\build\gui\file-browser.exe file-browser.exe 
copy ..\build\gui\file-browser.pdb file-browser.pdb 

%QTDIR%\bin\windeployqt.exe  --qmldir %SRC%\gui\qml file-browser.exe

copy %LIBARCHIVE%\bin\archive.dll archive.dll

cd ..

PAUSE
