@echo off

::setlocal

:: Build with the Windows Driver Kit, as recommended by Microsoft TSF team.
:: This eliminates the dependency on the Visual C++ Redistributable, and
:: instead links with the system VC++ Redistributable.
::

if not defined DDKBUILDENV goto error_ddk

pushd PinyinTones

set APPVER=6.0
set TARGET_OS=WLH

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: First, build the x64 version 
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

set DDKBUILDENV=
pushd \
call %BASEDIR%\bin\setenv.bat %BASEDIR% fre %TARGET_OS% x64
popd
set 386=
set CPU=AMD64

:: Include path:
::   - Windows SDK for win32.mak
::   - Visual Studio 10.0 for excpt.h
set INCLUDE=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\INCLUDE;%BASEDIR%\inc\api;%BASEDIR%\inc\crt


:: The DDK's setenv.cmd doesn't set LIB properly.  We have to add the
:: paths to kernel32.lib and msvcrt.lib so that the linker can find it.
set LIB=%DDK_LIB_DEST%\%CPU%;%LIB%\Crt\%CPU%;%LIB%;

:: Build PinyinTones
nmake nodebug=1
if not %ERRORLEVEL%==0 goto end

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Now build the x86 version
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

set DDKBUILDENV=
pushd \
call %BASEDIR%\bin\setenv.bat %BASEDIR% fre %TARGET_OS% x86
popd
set AMD64=
set CPU=i386
set INCLUDE=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\INCLUDE;%BASEDIR%\inc\api;%BASEDIR%\inc\crt
set LIB=%DDK_LIB_DEST%\%CPU%;%LIB%\Crt\%CPU%;%LIB%;
nmake nodebug=1
if not %ERRORLEVEL%==0 goto end

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build MSIs from WIX
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cd ..\Setup
candle -arch x86 -dPlatform=x86 -o ..\bin\PinyinTones32.wixobj PinyinTones.wxs 
candle -arch x64 -dPlatform=x64 -o ..\bin\PinyinTones64.wixobj PinyinTones.wxs 
cd ..\bin
light PinyinTones32.wixobj
light PinyinTones64.wixobj

if exist PinyinTones32.wixobj del PinyinTones32.wixobj
if exist PinyinTones64.wixobj del PinyinTones64.wixobj

popd
goto end

:error_ddk
echo This build script must be run from a DDK command prompt.
goto end

:end
endlocal