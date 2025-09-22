:: build.bat 建議內容（UTF-8存檔）
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set CFLAGS=/nologo /std:c++17 /utf-8 /EHsc /W3 /MT
set INCLUDES=/Iinclude

:: 乾淨化（可選）
del /q src\*.obj 2>nul

cl %CFLAGS% src\*.cpp %INCLUDES% /Fe:zhcl_universal.exe
echo Error level: %ERRORLEVEL%
