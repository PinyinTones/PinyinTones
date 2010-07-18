copy config.txt ..\bin\Win32
pushd ..\bin\win32
"C:\Program Files\7-Zip\7z.exe" a PinyinTones32.7z PinyinTones32.msi setup.exe
copy /b "C:\Program Files\7-Zip\7zs.sfx" + config.txt + PinyinTones32.7z PinyinTones32.exe
popd