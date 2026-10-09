@echo off

REM Create build directory if it doesn't exist
if not exist build (
    mkdir build
)

REM Change into build directory
cd build

REM Run CMake configure step with Visual Studio 17 2022 generator
"D:\drivers.gpu.compiler.raytracing-library\tools\cmake\bin\cmake.exe" -G "Visual Studio 17 2022" ..

REM Check if configuration succeeded
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

echo Setup succeeded!
pause
