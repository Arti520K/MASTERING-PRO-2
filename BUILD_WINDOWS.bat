@echo off
title MASTERING PRO 2.0 Build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto fail
cmake --build build --config Release
if errorlevel 1 goto fail
echo.
echo Build finished. Check build\MASTERING_PRO_2_artefacts\Release\VST3
pause
exit /b 0
:fail
echo Build failed. Install Visual Studio 2022 C++ workload, CMake, and Git.
pause
exit /b 1
