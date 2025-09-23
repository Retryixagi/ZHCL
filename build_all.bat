:: build_all.bat - Complete build process with codegen
@echo off
setlocal

:: === Generate zh keyword mappings ===
echo Skipping Chinese keyword mappings generation (now in chinese.h)...

:: === Compilation phase ===
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set CFLAGS=/nologo /std:c++17 /utf-8 /EHsc /W3 /MT
set INCLUDES=/Iinclude

:: Clean up (optional)
del /q src\*.obj 2>nul

cl %CFLAGS% src\fe_*.cpp src\frontend.cpp src\zh_frontend.cpp src\zh_glue.cpp src\zhcl_universal.cpp %INCLUDES% /Fe:zhcl_universal.exe
echo Build error level: %ERRORLEVEL%

endlocal
