@echo off
setlocal

set MSVC_ROOT=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207
set WIN_SDK=C:\Program Files (x86)\Windows Kits\10
set SDK_VER=10.0.26100.0

set PATH=%MSVC_ROOT%\bin\HostX64\x64;%WIN_SDK%\bin\%SDK_VER%\x64;%PATH%
set INCLUDE=%MSVC_ROOT%\include;%WIN_SDK%\Include\%SDK_VER%\ucrt;%WIN_SDK%\Include\%SDK_VER%\um;%WIN_SDK%\Include\%SDK_VER%\shared
set LIB=%MSVC_ROOT%\lib\x64;%WIN_SDK%\Lib\%SDK_VER%\ucrt\x64;%WIN_SDK%\Lib\%SDK_VER%\um\x64

set CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
set NINJA=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe

cd /d "%~dp0"
if exist build rmdir /s /q build

echo Configuring...
"%CMAKE%" -B build -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%NINJA%"
if %ERRORLEVEL% NEQ 0 (
    echo CONFIGURE FAILED
    exit /b 1
)

echo Building...
"%CMAKE%" --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo BUILD FAILED
    exit /b 1
)

echo SUCCESS
dir build\bin\*.dll 2>nul
dir build\bin\*.lib 2>nul
