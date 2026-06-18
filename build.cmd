@echo off
rem build.cmd - configure + build both targets with the static-CRT MSVC toolchain.
rem Uses the Visual Studio generator (no Ninja-in-PATH dependency).
setlocal EnableDelayedExpansion
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "CMAKE=C:\Program Files\CMake\bin\cmake.exe"
if not exist "%CMAKE%" set "CMAKE=cmake"
set "BUILD=%ROOT%\build\by-agent"

"%CMAKE%" -S "%ROOT%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 ( echo [build] configure failed & exit /b 1 )

"%CMAKE%" --build "%BUILD%" --config Release
if errorlevel 1 ( echo [build] build failed & exit /b 1 )

set "GUI=%BUILD%\apps\timer-res\Release\timer-res.exe"
set "TOOL=%BUILD%\apps\timer-res-test-tool\Release\timer-res-test-tool.exe"
echo [build] OK
for %%S in ("%GUI%")  do echo   timer-res.exe           %%~zS bytes  (%GUI%)
for %%S in ("%TOOL%") do echo   timer-res-test-tool.exe %%~zS bytes  (%TOOL%)
endlocal
