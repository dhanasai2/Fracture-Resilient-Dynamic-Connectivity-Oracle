@echo off
echo ============================================
echo  SHATTERMAP BENCHMARK - Build and Run
echo ============================================
echo.

REM Try g++ first
where g++ >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo [BUILD] Compiling with g++...
    g++ -O2 -std=c++17 -o shattermap_benchmark.exe shattermap_benchmark.cpp
    if %ERRORLEVEL% equ 0 (
        echo [BUILD] Compilation successful!
        echo [RUN] Starting benchmark...
        echo.
        shattermap_benchmark.exe
    ) else (
        echo [ERROR] Compilation failed!
    )
) else (
    echo [INFO] g++ not found, trying cl (MSVC)...
    where cl >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        cl /EHsc /O2 /std:c++17 /Fe:shattermap_benchmark.exe shattermap_benchmark.cpp
        if %ERRORLEVEL% equ 0 (
            echo [BUILD] Compilation successful!
            echo [RUN] Starting benchmark...
            shattermap_benchmark.exe
        )
    ) else (
        echo [ERROR] No C++ compiler found! Install MinGW g++ or MSVC.
    )
)
echo.
pause
