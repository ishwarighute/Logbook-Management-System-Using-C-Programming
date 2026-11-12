@echo off
echo =================================================================
echo             LOGBOOK APPLICATION - BUILD AND TEST
echo =================================================================

echo [1/4] Compiling Modular C Application (logbook_c.exe)...
gcc -Wall -Wextra -std=c11 logbook.c main.c -o logbook_c.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to compile logbook_c.exe
    exit /b %ERRORLEVEL%
)

echo [2/4] Compiling Single-File C Version (logbook_single.exe)...
gcc -Wall -Wextra -std=c11 logbook_single.c -o logbook_single.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to compile logbook_single.exe
    exit /b %ERRORLEVEL%
)

echo [3/4] Compiling Modern C++ Version (logbook_cpp.exe)...
g++ -Wall -Wextra -std=c++17 logbook.cpp -o logbook_cpp.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to compile logbook_cpp.exe
    exit /b %ERRORLEVEL%
)

echo [4/4] Compiling Test Suite (test_runner.exe)...
gcc -Wall -Wextra -std=c11 logbook.c test_runner.c -o test_runner.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to compile test_runner.exe
    exit /b %ERRORLEVEL%
)

echo.
echo =================================================================
echo [SUCCESS] All executables compiled successfully!
echo =================================================================
echo.
echo Running automated test suite...
.\test_runner.exe
echo.
echo Build and test run complete!

