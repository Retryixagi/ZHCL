@echo off
setlocal
if "%~2"=="" (
  echo �Ϊk: build_windows_msvc.bat input.zh output.exe
  exit /b 1
)
set INPUT=%~1
set OUTPUT=%~2
where cl >nul 2>nul
if errorlevel 1 (
  echo ����� cl.exe�A�бq Developer Command Prompt ����C
  exit /b 1
)
python zhcc.py "%INPUT%" -o "%OUTPUT%"
if errorlevel 1 exit /b 1
echo �����G%OUTPUT%
