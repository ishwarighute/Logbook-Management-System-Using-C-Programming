# Task 3: Logbook Application

A production-grade, memory-efficient **Logbook Application** built in C (with additional modern C++ and Python implementations), fulfilling all requirements of **Task 3: File I/O & Time Functions**.

---

## 📌 Project Overview

This project implements a persistent logbook system that:
1. **Appends Timestamped Entries**: Records user-submitted events or messages to a persistent disk file with second-precision timestamps (`YYYY-MM-DD HH:MM:SS`) using standard C time functions and append-mode file I/O.
2. **Displays the Last $N$ Entries**: Efficiently extracts and presents the most recent $N$ entries in chronological order using an $O(N)$ sliding-window ring buffer algorithm without loading the entire file into memory.
3. **Advanced Features**:
   - Optional log levels / category tags (e.g., `[INFO]`, `[WARN]`, `[ERROR]`, `[DEBUG]`).
   - Case-insensitive full-text substring search across historical entries.
   - Real-time logbook metadata and metrics (entry count, file size in bytes/KB, earliest and latest recorded timestamps).
   - Safe log file truncation / rotation.
   - Dual interface modes: Rich interactive CLI menu and non-interactive command-line argument flags for terminal scripting and automated pipelines.

---

## 📁 Repository Structure

```
c:\logbook\
├── logbook.h              # Modular C public API header with comprehensive prototypes
├── logbook.c              # Modular C core engine (File I/O, Time formatting, Ring Buffer)
├── main.c                 # Modular C CLI & interactive menu entry point
├── test_runner.c          # Automated test suite (7 suites, 46 assertions, 100% pass)
├── logbook_single.c       # Self-contained single-file C solution (for course submissions)
├── logbook.cpp            # Modern C++ (C++17) implementation with RAII, STL, and std::chrono
├── logbook.py             # Python 3 reference implementation with argparse and collections.deque
├── Makefile               # MinGW Makefile for compilation, testing, and cleanup
├── build.bat              # One-click Windows build and test script
└── README.md              # Project documentation and specifications
```

---

## 🧠 System Architecture & Core Concepts

### 1. File I/O Mechanics (Append Mode vs Truncate)
When appending entries, the application opens the log file using the `"a"` mode:
```c
FILE *fp = fopen(lb->filepath, "a");
```
- **Guaranteed Atomic Appends**: Under `"a"` mode, all write operations automatically position the file pointer at the end of the file, preventing overwrites of historical entries even if multiple threads or processes append concurrently.
- **Immediate Disk Flush**: Calls to `fflush(fp)` immediately follow each write to guarantee data persistence before the file descriptor is closed, preventing loss during power outages or unexpected process exits.

### 2. Time Functions (`<time.h>`)
Timestamp generation adheres strictly to standard C time APIs:
```
           +-------------+
           | time(NULL)  |  -> Retrieves epoch seconds (time_t)
           +------+------+
                  |
                  v
          +---------------+
          |  localtime()  |  -> Converts epoch seconds to broken-down calendar time (struct tm)
          +-------+-------+
                  |
                  v
          +---------------+
          |  strftime()   |  -> Formats calendar time into ISO-8601: "YYYY-MM-DD HH:MM:SS"
          +---------------+
```

Formatted entry output structure:
```
[YYYY-MM-DD HH:MM:SS] [OPTIONAL_TAG] Log message text here
```

### 3. Ring-Buffer Algorithm for Last $N$ Entries
Naive solutions read the entire file into memory or perform multiple file passes. This implementation utilizes a **Fixed-Capacity Ring Buffer (Circular Queue)** of size $N$:

```
Reading lines sequentially with N = 3:

Line 1 arrives:   [ Line 1 ] [   -    ] [   -    ]   (head = 0, count = 1)
Line 2 arrives:   [ Line 1 ] [ Line 2 ] [   -    ]   (head = 0, count = 2)
Line 3 arrives:   [ Line 1 ] [ Line 2 ] [ Line 3 ]   (head = 0, count = 3)
Line 4 arrives:   [ Line 4 ] [ Line 2 ] [ Line 3 ]   (head = 1, evicted Line 1)
Line 5 arrives:   [ Line 4 ] [ Line 5 ] [ Line 3 ]   (head = 2, evicted Line 2)

Final Reassembly:
Index 0 -> slot (head + 0) % 3 = slot 2 -> Line 3
Index 1 -> slot (head + 1) % 3 = slot 0 -> Line 4
Index 2 -> slot (head + 2) % 3 = slot 1 -> Line 5
```

- **Memory Bound**: Memory usage is strictly bounded by $O(N \times \text{line\_length})$, regardless of whether the log file contains 10 lines or 10,000,000 lines.
- **Single Pass**: The file is scanned only once ($O(M)$ time where $M$ is total lines in the file).

---

## ⏱️ Complexity Analysis

| Operation | Function | Time Complexity | Auxiliary Space Complexity | Description |
|---|---|:---:|:---:|---|
| **Append Entry** | `logbook_append` | $O(1)$ | $O(1)$ | Opens in `"a"` mode, seeks to EOF, writes line |
| **Tagged Append** | `logbook_append_with_tag` | $O(1)$ | $O(1)$ | Formats tag and message, atomic write |
| **Count Entries** | `logbook_count_entries` | $O(M)$ | $O(1)$ | Single sequential scan through file ($M$ lines) |
| **Get Last $N$ Entries**| `logbook_get_last_n` | $O(M)$ | $O(N)$ | Circular buffer rollover; retains only $N$ lines |
| **Display All** | `logbook_display_all` | $O(M)$ | $O(1)$ | Direct streaming to stdout |
| **Search Keyword** | `logbook_search` | $O(M \times L)$ | $O(1)$ | Case-insensitive substring match per line |
| **Get Stats** | `logbook_get_stats` | $O(M)$ | $O(1)$ | Retrieves size via `stat` and parsed timestamps |
| **Clear Log** | `logbook_clear` | $O(1)$ | $O(1)$ | Truncates file via `"w"` mode |

---

## 🛠️ Build and Execution Guide

### Option 1: One-Click Windows Batch Script
```powershell
.\build.bat
```
*Compiles all executables (`logbook_c.exe`, `logbook_single.exe`, `logbook_cpp.exe`, `test_runner.exe`) and automatically runs the 46-assertion test suite.*

### Option 2: MinGW Make
```powershell
# Build all targets
mingw32-make all

# Run automated unit & integration tests
mingw32-make test

# Run Python verification
mingw32-make test_py

# Launch interactive C application
mingw32-make run

# Clean build artifacts
mingw32-make clean
```

### Option 3: Manual GCC Compilation
```powershell
# Modular C Application
gcc -Wall -Wextra -std=c11 logbook.c main.c -o logbook_c.exe

# Single-file standalone version (for course submission)
gcc -Wall -Wextra -std=c11 logbook_single.c -o logbook_single.exe

# Modern C++ version
g++ -Wall -Wextra -std=c++17 logbook.cpp -o logbook_cpp.exe

# Test suite
gcc -Wall -Wextra -std=c11 logbook.c test_runner.c -o test_runner.exe
```

---

## 💻 Usage & CLI Reference

### 1. Interactive Menu Mode
Simply launch any executable without arguments:
```powershell
.\logbook_c.exe
```
```
================================================================
          LOGBOOK APPLICATION (File I/O & Time Functions)       
================================================================

--- LOGBOOK MENU (Target File: logbook.txt) ---
  [1] Append New Timestamped Entry
  [2] Append Entry with Tag / Level (INFO, WARN, ERROR, etc.)
  [3] Display Last N Entries
  [4] Display All Entries
  [5] Search Entries by Keyword
  [6] View Logbook Statistics
  [7] Clear / Reset Logbook
  [8] Change Target Log File
  [0] Exit
----------------------------------------------------------------
Enter choice (0-8):
```

### 2. Command-Line Arguments Mode
The tool can be scripted directly from PowerShell / CMD / Bash:

```powershell
# Append a standard timestamped entry
.\logbook_c.exe --add "System backup completed successfully"

# Append a tagged entry (e.g. WARN, ERROR, INFO)
.\logbook_c.exe --add "High disk usage detected on volume D:" --tag "WARN"

# Display the last 5 entries
.\logbook_c.exe --last 5

# Display all entries in the logbook
.\logbook_c.exe --all

# Search historical entries for a keyword
.\logbook_c.exe --search "backup"

# View logbook statistics (entry count, file size, date range)
.\logbook_c.exe --stats

# Target a custom log file
.\logbook_c.exe --add "Custom worker deployed" --file "worker.log"
.\logbook_c.exe --last 3 --file "worker.log"

# Clear log file
.\logbook_c.exe --clear
```

### 3. Python Reference Usage
```powershell
# Interactive mode
python logbook.py

# CLI mode
python logbook.py --add "Server heartbeat OK" --tag "HEALTH"
python logbook.py --last 3
python logbook.py --stats
```

---

## 🧪 Automated Test Suite

The test suite (`test_runner.c`) executes 46 assertions across 7 comprehensive test suites:

| Suite | Focus Area | Assertions | Status |
|---|---|:---:|:---:|
| **Suite 1** | Lifecycle, file initialization, non-existent file handling | 5 | ✅ 100% Pass |
| **Suite 2** | Timestamp formatting (`YYYY-MM-DD HH:MM:SS` digit/delimiter validation) | 1 | ✅ 100% Pass |
| **Suite 3** | File append mode persistence, multiple appends, tagged entries | 6 | ✅ 100% Pass |
| **Suite 4** | Last $N$ retrieval ($N < \text{total}$, $N = \text{total}$, $N > \text{total}$, $N \le 0$) | 13 | ✅ 100% Pass |
| **Suite 5** | Case-insensitive keyword search, metadata & timestamp extraction | 7 | ✅ 100% Pass |
| **Suite 6** | Log clearing / truncation and subsequent re-append | 6 | ✅ 100% Pass |
| **Suite 7** | Stress rollover test (100 sequential entries, verifying last 5 ordering) | 8 | ✅ 100% Pass |
| **TOTAL** | **Full Verification** | **46 / 46** | **✅ 100% Pass** |

