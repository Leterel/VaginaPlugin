@echo off
cd /d "%~dp0"
cmake -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
echo.
echo FERTIG:
echo build\VaginaPlugin_artefacts\Release\VST3\Vagina Squirt.vst3
echo.
echo Kopieren nach: C:\Program Files\Common Files\VST3\
pause
