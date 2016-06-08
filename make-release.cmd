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

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build and sign PinyinTones
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: Build and sign DLLs
call :build_dll x86 i386
call :build_dll x64 AMD64

:: Build and sign MSIs
call :build_msi x86 32
call :build_msi x64 64

:: Done
echo.
echo -------------------------------------
echo Release build complete.
echo -------------------------------------
goto end


:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build PinyinTones DLL
::   %1 = Platform (x86 or x64)
::   %2 = CPU name (i386 or AMD64)
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:build_dll
setlocal

:: Clear to fool setenv into running a second time
set DDKBUILDENV=

:: Clear other DDK environment variables
set 386=
set AMD64=

:: Set environment for the desired target.  The batch file resets the current
:: directory, so we have to restore it after the call.
pushd \
call %BASEDIR%\bin\setenv.bat %BASEDIR% fre %TARGET_OS% %1
popd

pushd PinyinTones
set PUSHED_DIR=1

set CPU=%2

:: Include path:
::   - Windows SDK for win32.mak
set INCLUDE=C:\Program Files\Microsoft SDKs\Windows\v7.1\INCLUDE;%BASEDIR%\inc\api;%BASEDIR%\inc\crt


:: The DDK's setenv.cmd doesn't set LIB properly.  We have to add the
:: paths to kernel32.lib and msvcrt.lib so that the linker can find it.
set LIB=%DDK_LIB_DEST%\%CPU%;%LIB%\Crt\%CPU%;%LIB%;

:: Build PinyinTones
nmake nodebug=1
if not %ERRORLEVEL%==0 goto end

:: Sign DLL
set DLL_DIR=%TARGET_OS_NAME%_RETAIL
if %1==x64 set DLL_DIR=%TARGET_OS_NAME%_X64_RETAIL
call :sign ..\bin\%DLL_DIR%\PinyinTones.dll "%DESC%"

popd
set PUSHED_DIR=

endlocal
goto :eof


:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build and sign MSI
::   %1 = Platform (x86 or x64)
::   %2 = Suffix (e.g. 32 for PinyinTones32.msi)
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:build_msi
pushd Setup
set PUSHED_DIR=1

candle -arch %1 -dPlatform=%1 -o ..\bin\PinyinTones%2.wixobj PinyinTones.wxs 
if not %ERRORLEVEL%==0 goto end

popd
pushd bin
set PUSHED_DIR=1
light -ext WixUIExtension PinyinTones%2.wixobj
if not %ERRORLEVEL%==0 goto end

call :sign PinyinTones%2.msi "%DESC%"
if exist PinyinTones%2.wixobj del PinyinTones%2.wixobj

popd
set PUSHED_DIR=
goto :eof


:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Sign file:
::   %1 = filename
::   %2 = description
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:sign
signtool sign ^
  /f "%CERT%" /p "%CERT_PASSWD%" /d "%~2" /du "%URL%" %TIMESTAMP_OPTION% ^
  %1
if not %ERRORLEVEL%==0 goto end
goto :eof

:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Terminate command script
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 
:error_signing_script
echo You must create a set-signing-cert.cmd file to provide the
echo CERT and CERT_PASSWD environment variables.
goto end

:error_ddk
echo This build script must be run from a DDK command prompt.
goto end

:end
if defined PUSHED_DIR popd
endlocal