@echo off
setlocal
echo ===================================================================
echo  Building Disaster Relief ^& Resource Management System
echo  Backend: C++ / Crow / SQLite3
echo ===================================================================

if not exist "data" mkdir data
if not exist "build" mkdir build

echo [1/2] Compiling SQLite3 database engine (C)...
gcc -O2 -Iinclude -c src\sqlite3.c -o build\sqlite3.o
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to compile sqlite3.c
    if "%1" neq "--no-pause" pause
    exit /b 1
)

echo [2/2] Compiling C++ Crow server and linking...
g++ -std=c++17 -O2 -Iinclude -D_WIN32_WINNT=0x0601 ^
    src\main.cpp ^
    src\database.cpp ^
    src\disaster.cpp ^
    src\resource.cpp ^
    src\relief_request.cpp ^
    src\distribution.cpp ^
    src\system.cpp ^
    build\sqlite3.o ^
    -lws2_32 -lmswsock -lpthread ^
    -o DisasterReliefSystem.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ===================================================================
    echo  Build Successful: DisasterReliefSystem.exe
    echo  To launch the server, run: DisasterReliefSystem.exe
    echo  Then open http://localhost:8080 in your web browser.
    echo ===================================================================
) else (
    echo.
    echo [ERROR] Compilation failed.
)

if "%1" neq "--no-pause" pause
