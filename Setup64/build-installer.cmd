copy config.txt ..\bin\x64
pushd ..\bin\x64
"C:\Program Files\7-Zip\7z.exe" a PinyinTones64.7z PinyinTones64.msi setup.exe
copy /b "C:\Program Files\7-Zip\7zs.sfx" + config.txt + PinyinTones64.7z PinyinTones64.exe
popd