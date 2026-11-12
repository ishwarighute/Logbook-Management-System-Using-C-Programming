# Makefile for Logbook Application
# Supports MinGW / GCC on Windows

CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c11 -O2
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
PYTHON = python

TARGETS = logbook_c.exe logbook_single.exe logbook_cpp.exe test_runner.exe

.PHONY: all clean test test_py help run

all: $(TARGETS)

logbook_c.exe: logbook.c main.c logbook.h
	$(CC) $(CFLAGS) logbook.c main.c -o $@

logbook_single.exe: logbook_single.c
	$(CC) $(CFLAGS) logbook_single.c -o $@

logbook_cpp.exe: logbook.cpp
	$(CXX) $(CXXFLAGS) logbook.cpp -o $@

test_runner.exe: logbook.c test_runner.c logbook.h
	$(CC) $(CFLAGS) logbook.c test_runner.c -o $@

test: test_runner.exe
	@echo [RUNNING UNIT & INTEGRATION TESTS]
	.\test_runner.exe

test_py:
	@echo [TESTING PYTHON IMPLEMENTATION]
	$(PYTHON) -c "import logbook; lb = logbook.LogBook('test_py.log'); lb.clear(); lb.append('Test Py 1'); lb.append('Test Py 2'); assert len(lb.get_last_n(1)) == 1; lb.clear(); print('Python verification successful!')"

run: logbook_c.exe
	.\logbook_c.exe

clean:
	@if exist logbook_c.exe del /f /q logbook_c.exe
	@if exist logbook_single.exe del /f /q logbook_single.exe
	@if exist logbook_cpp.exe del /f /q logbook_cpp.exe
	@if exist test_runner.exe del /f /q test_runner.exe
	@if exist *.log del /f /q *.log
	@if exist test_demo.log del /f /q test_demo.log
	@echo [CLEAN COMPLETE]

help:
	@echo Available targets:
	@echo   make all      - Compile C, C++, and test executables
	@echo   make test     - Compile and run test suite
	@echo   make test_py  - Run Python verification test
	@echo   make run      - Run modular C interactive application
	@echo   make clean    - Remove executables and temporary log files

