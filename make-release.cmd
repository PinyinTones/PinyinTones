@echo off

:: Build with the Windows Driver Kit, as recommended by Microsoft TSF team.
:: This eliminates the dependency on the Visual C++ Redistributable, and
:: instead links with the system VC++ Redistributable.
::
:: code-sign.cmd should be created to call signtool with the appropriate
:: certificate and password, passing through all remaining parameters.

if not defined DDKBUILDENV goto error_ddk

setlocal
set PUSHED_DIR=
set APPVER=6.0
set TARGET_OS=WLH
set TARGET_OS_NAME=Vista
set DESC=PinyinTones text service
set URL=http://pinyintones.codeplex.com/
set TIMESTAMP_OPTION=/t http://timestamp.verisign.com/scripts/timstamp.dll 

:: CERT and CERT_PASSWD are supplied by local script.  Script can also
:: opt to avoid timestamping by clearing out the whole option.
set CERT=
set CERT_PASSWD=
if not exist set-signing-cert.cmd goto error_signing_script
call set-signing-cert.cmd

:: Clear to fool setenv into running a second time
set DDKBUILDENV=

:: Clear other DDK environment variables
set 386=
set AMD64=

pushd PinyinTones
set PUSHED_DIR=1

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: First, build the x64 version 
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

setlocal

:: Set environment for the desired target.  The batch file resets the current
:: directory, so we have to restore it after the call.
pushd \
call %BASEDIR%\bin\setenv.bat %BASEDIR% fre %TARGET_OS% x64
popd
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

:: Sign it
set DESC=%DESC% (64-bit)
signtool sign ^
  /f "%CERT%" /p "%CERT_PASSWD%" /d "%DESC%" /du "%URL%" %TIMESTAMP_OPTION% ^
  ..\bin\%TARGET_OS_NAME%_X64_RETAIL\PinyinTones.dll
if not %ERRORLEVEL%==0 goto end
endlocal

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Now build the x86 version
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

setlocal
pushd \
call %BASEDIR%\bin\setenv.bat %BASEDIR% fre %TARGET_OS% x86
popd

set CPU=i386
set INCLUDE=C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\INCLUDE;%BASEDIR%\inc\api;%BASEDIR%\inc\crt
set LIB=%DDK_LIB_DEST%\%CPU%;%LIB%\Crt\%CPU%;%LIB%;
nmake nodebug=1
if not %ERRORLEVEL%==0 goto end
set DESC=%DESC% (32-bit)
signtool sign ^
  /f "%CERT%" /p "%CERT_PASSWD%" /d "%DESC%" /du "%URL%" %TIMESTAMP_OPTION% ^
  ..\bin\%TARGET_OS_NAME%_RETAIL\PinyinTones.dll
if not %ERRORLEVEL%==0 goto end
endlocal

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build MSIs from WIX
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

cd ..\Setup
candle -arch x86 -dPlatform=x86 -o ..\bin\PinyinTones32.wixobj PinyinTones.wxs 
if not %ERRORLEVEL%==0 goto end
candle -arch x64 -dPlatform=x64 -o ..\bin\PinyinTones64.wixobj PinyinTones.wxs 
if not %ERRORLEVEL%==0 goto end

cd ..\bin
light PinyinTones32.wixobj
if not %ERRORLEVEL%==0 goto end
light PinyinTones64.wixobj
if not %ERRORLEVEL%==0 goto end

:: Sign both MSIs
signtool sign ^
  /f "%CERT%" /p "%CERT_PASSWD%" /d "%DESC%" /du "%URL%" %TIMESTAMP_OPTION% ^
  PinyinTones32.msi
if not %ERRORLEVEL%==0 goto end

signtool sign ^
  /f "%CERT%" /p "%CERT_PASSWD%" /d "%DESC%" /du "%URL%" %TIMESTAMP_OPTION% ^
  PinyinTones64.msi
if not %ERRORLEVEL%==0 goto end

if exist PinyinTones32.wixobj del PinyinTones32.wixobj
if exist PinyinTones64.wixobj del PinyinTones64.wixobj

echo.
echo -------------------------------------
echo Release build complete.
echo -------------------------------------
goto end

:error_signing_script
echo You must create a set-signing-cert.cmd file to provide the
echo CERT and CERT_PASSWD environment variables.
goto end

:error_ddk
echo This build script must be run from a DDK command prompt.
goto end

:end
if defined %PUSHED_DIR% popd
endlocal