#!/usr/bin/env python3
"""
logbook.py - Complete Python Reference Implementation for the Logbook Application.

Features:
- Appends timestamped entries to a log file using standard datetime and file I/O ('a' mode)
- Retrieves and displays the last N entries using collections.deque with O(N) memory
- Supports optional tags/levels ([INFO], [WARN], [ERROR])
- Search, statistics, and log rotation/clear capabilities
- Both rich CLI argument mode and interactive terminal menu mode
"""

import sys
import os
import argparse
from datetime import datetime
from collections import deque

DEFAULT_LOG_FILE = "logbook.txt"

class LogBook:
    def __init__(self, filepath: str = DEFAULT_LOG_FILE):
        self.filepath = filepath

    def append(self, message: str, tag: str = "") -> bool:
        """Appends a timestamped log entry to the log file."""
        now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        clean_msg = message.strip()
        if not clean_msg:
            return False

        entry = f"[{now_str}] [{tag.strip()}] {clean_msg}\n" if tag.strip() else f"[{now_str}] {clean_msg}\n"

        try:
            with open(self.filepath, "a", encoding="utf-8") as f:
                f.write(entry)
                f.flush()
            return True
        except IOError as e:
            print(f"[ERROR] Failed to write to {self.filepath}: {e}", file=sys.stderr)
            return False

    def count_entries(self) -> int:
        """Counts valid entries in the log file."""
        if not os.path.exists(self.filepath):
            return 0
        count = 0
        with open(self.filepath, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if line.strip():
                    count += 1
        return count

    def get_last_n(self, n: int) -> list:
        """Returns the last N entries using a sliding window deque with O(N) space."""
        if n <= 0 or not os.path.exists(self.filepath):
            return []

        ring = deque(maxlen=n)
        with open(self.filepath, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                stripped = line.strip()
                if stripped:
                    ring.append(stripped)
        return list(ring)

    def display_last_n(self, n: int) -> int:
        """Displays the last N entries formatted nicely."""
        if n <= 0:
            print(f"[LOGBOOK] Invalid N={n}. Please provide N >= 1.")
            return 0

        entries = self.get_last_n(n)
        if not entries:
            print("\n+--------------------------------------------------------------+")
            print("| [LOGBOOK] Log file is currently empty or does not exist.     |")
            print("+--------------------------------------------------------------+\n")
            return 0

        total = self.count_entries()
        print("\n================================================================")
        print(f"                 LAST {len(entries)} LOGBOOK ENTRIES (Python)            ")
        if total > len(entries):
            print(f"       (Showing {len(entries)} of {total} total entries in chronological order)")
        else:
            print(f"                (Showing all {len(entries)} available entries)")
        print("================================================================")
        for i, entry in enumerate(entries, 1):
            print(f"[{i:3d}] {entry}")
        print("================================================================\n")
        return len(entries)

    def display_all(self) -> int:
        """Displays all entries from start to end."""
        if not os.path.exists(self.filepath):
            print(f"\n[LOGBOOK] File '{self.filepath}' not found.\n")
            return 0

        count = 0
        print("\n================================================================")
        print(f"                   ALL LOGBOOK ENTRIES ({self.filepath})")
        print("================================================================")
        with open(self.filepath, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                stripped = line.strip()
                if stripped:
                    count += 1
                    print(f"[{count:4d}] {stripped}")
        print("================================================================")
        print(f" Total entries displayed: {count}\n")
        return count

    def search(self, keyword: str) -> int:
        """Searches entries containing keyword case-insensitively."""
        if not os.path.exists(self.filepath):
            print(f"[LOGBOOK] File '{self.filepath}' not found.")
            return 0

        kw = keyword.lower()
        matches = 0
        line_num = 0
        print("\n================================================================")
        print(f" SEARCH RESULTS FOR: \"{keyword}\"")
        print("================================================================")
        with open(self.filepath, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line_num += 1
                stripped = line.strip()
                if kw in stripped.lower():
                    matches += 1
                    print(f"Line {line_num:4d}: {stripped}")
        print("================================================================")
        print(f" Found {matches} match(es) across {line_num} lines.\n")
        return matches

    def clear(self) -> bool:
        """Clears/truncates the log file."""
        try:
            with open(self.filepath, "w", encoding="utf-8") as f:
                f.truncate(0)
            return True
        except IOError as e:
            print(f"[ERROR] Could not clear file: {e}", file=sys.stderr)
            return False

    def stats(self):
        """Displays logbook statistics."""
        total = self.count_entries()
        size_bytes = os.path.getsize(self.filepath) if os.path.exists(self.filepath) else 0
        first_ts = "N/A"
        last_ts = "N/A"

        if os.path.exists(self.filepath):
            with open(self.filepath, "r", encoding="utf-8", errors="replace") as f:
                idx = 0
                for line in f:
                    s = line.strip()
                    if s.startswith("[") and "]" in s:
                        ts = s[1:s.index("]")]
                        if idx == 0:
                            first_ts = ts
                        last_ts = ts
                        idx += 1

        print("\n================================================================")
        print("                 LOGBOOK STATISTICS (Python)                    ")
        print("================================================================")
        print(f" Log File:         {self.filepath}")
        print(f" Total Entries:    {total}")
        print(f" File Size:        {size_bytes} bytes ({size_bytes / 1024.0:.2f} KB)")
        print(f" First Entry Date: {first_ts}")
        print(f" Last Entry Date:  {last_ts}")
        print("================================================================\n")


def interactive_menu(lb: LogBook):
    print("================================================================")
    print("           LOGBOOK APPLICATION (Python Reference)              ")
    print("================================================================")

    while True:
        print(f"\n--- LOGBOOK MENU ({lb.filepath}) ---")
        print("  [1] Append New Timestamped Entry")
        print("  [2] Append Entry with Tag (INFO, WARN, ERROR, etc.)")
        print("  [3] Display Last N Entries")
        print("  [4] Display All Entries")
        print("  [5] Search Entries by Keyword")
        print("  [6] View Logbook Statistics")
        print("  [7] Clear Logbook")
        print("  [0] Exit")
        choice = input("Enter choice (0-7): ").strip()

        if choice == "0":
            print("Exiting Logbook. Goodbye!")
            break
        elif choice == "1":
            msg = input("Enter log message: ").strip()
            if msg:
                if lb.append(msg):
                    print("[✓] Entry recorded successfully!")
            else:
                print("[!] Message cannot be empty.")
        elif choice == "2":
            tag = input("Enter tag (INFO, ERROR, etc.): ").strip()
            msg = input("Enter log message: ").strip()
            if msg:
                if lb.append(msg, tag):
                    print("[✓] Tagged entry recorded successfully!")
            else:
                print("[!] Message cannot be empty.")
        elif choice == "3":
            n_str = input("Enter number of recent entries (N): ").strip()
            try:
                n = int(n_str)
                lb.display_last_n(n)
            except ValueError:
                print("[!] Please enter a valid positive integer.")
        elif choice == "4":
            lb.display_all()
        elif choice == "5":
            kw = input("Enter search keyword: ").strip()
            if kw:
                lb.search(kw)
        elif choice == "6":
            lb.stats()
        elif choice == "7":
            confirm = input(f"Are you sure you want to clear '{lb.filepath}'? (y/N): ").strip().lower()
            if confirm == "y":
                if lb.clear():
                    print("[✓] Logbook cleared.")
            else:
                print("[-] Operation cancelled.")
        else:
            print("[!] Invalid selection.")


def main():
    parser = argparse.ArgumentParser(description="Logbook Application - File I/O & Time Functions")
    parser.add_argument("-a", "--add", type=str, help="Append a new timestamped log entry")
    parser.add_argument("-t", "--tag", type=str, default="", help="Optional tag for the log entry")
    parser.add_argument("-n", "--last", type=int, help="Display the last N entries")
    parser.add_argument("-l", "--all", action="store_true", help="Display all log entries")
    parser.add_argument("-s", "--search", type=str, help="Search for keyword in entries")
    parser.add_argument("-S", "--stats", action="store_true", help="Display logbook statistics")
    parser.add_argument("-c", "--clear", action="store_true", help="Clear/truncate the log file")
    parser.add_argument("-f", "--file", type=str, default=DEFAULT_LOG_FILE, help="Target log file path")

    args = parser.parse_args()
    lb = LogBook(args.file)

    if len(sys.argv) == 1:
        interactive_menu(lb)
        return

    if args.clear:
        if lb.clear():
            print(f"[LOGBOOK] Cleared {lb.filepath}")

    if args.add:
        if lb.append(args.add, args.tag):
            print("[LOGBOOK] Entry appended successfully.")

    if args.last is not None:
        lb.display_last_n(args.last)

    if args.all:
        lb.display_all()

    if args.search:
        lb.search(args.search)

    if args.stats:
        lb.stats()


if __name__ == "__main__":
    main()

