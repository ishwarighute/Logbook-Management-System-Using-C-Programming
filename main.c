/**
 * @file main.c
 * @brief Interactive CLI and command-line argument driver for Logbook Application.
 */

#include "logbook.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_banner(void) {
    printf("================================================================\n");
    printf("          LOGBOOK APPLICATION (File I/O & Time Functions)       \n");
    printf("================================================================\n");
}

static void print_menu(const char *filepath) {
    printf("\n--- LOGBOOK MENU (Target File: %s) ---\n", filepath);
    printf("  [1] Append New Timestamped Entry\n");
    printf("  [2] Append Entry with Tag / Level (INFO, WARN, ERROR, etc.)\n");
    printf("  [3] Display Last N Entries\n");
    printf("  [4] Display All Entries\n");
    printf("  [5] Search Entries by Keyword\n");
    printf("  [6] View Logbook Statistics\n");
    printf("  [7] Clear / Reset Logbook\n");
    printf("  [8] Change Target Log File\n");
    printf("  [0] Exit\n");
    printf("----------------------------------------------------------------\n");
    printf("Enter choice (0-8): ");
}

static void read_line_stdin(char *buffer, size_t size) {
    if (!fgets(buffer, (int)size, stdin)) {
        buffer[0] = '\0';
        return;
    }
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        buffer[--len] = '\0';
    }
}

static void print_help(const char *prog_name) {
    printf("Usage:\n");
    printf("  Interactive mode: %s\n", prog_name);
    printf("  Command-line mode:\n");
    printf("    %s --add \"<message>\" [--tag <tag>] [--file <path>]\n", prog_name);
    printf("    %s --last <N> [--file <path>]\n", prog_name);
    printf("    %s --all [--file <path>]\n", prog_name);
    printf("    %s --search \"<keyword>\" [--file <path>]\n", prog_name);
    printf("    %s --stats [--file <path>]\n", prog_name);
    printf("    %s --clear [--file <path>]\n", prog_name);
    printf("    %s --help\n\n", prog_name);
    printf("Options:\n");
    printf("  -a, --add <msg>      Append a new timestamped log entry\n");
    printf("  -t, --tag <tag>      Optional tag/level to accompany --add\n");
    printf("  -n, --last <N>       Display the last N entries\n");
    printf("  -l, --all            Display all log entries\n");
    printf("  -s, --search <kw>    Search log entries for a keyword\n");
    printf("  -S, --stats          Display file statistics and timestamps\n");
    printf("  -c, --clear          Clear/truncate the log file\n");
    printf("  -f, --file <path>    Specify custom log file (default: logbook.txt)\n");
    printf("  -h, --help           Show this help message\n");
}

int main(int argc, char *argv[]) {
    char filepath[512] = DEFAULT_LOG_FILENAME;
    const char *add_msg = NULL;
    const char *tag = NULL;
    const char *search_kw = NULL;
    int last_n = -1;
    bool show_all = false;
    bool show_stats = false;
    bool do_clear = false;

    /* Parse CLI arguments if provided */
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                print_help(argv[0]);
                return 0;
            } else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) && i + 1 < argc) {
                strncpy(filepath, argv[++i], sizeof(filepath) - 1);
                filepath[sizeof(filepath) - 1] = '\0';
            } else if ((strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--add") == 0) && i + 1 < argc) {
                add_msg = argv[++i];
            } else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tag") == 0) && i + 1 < argc) {
                tag = argv[++i];
            } else if ((strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--last") == 0) && i + 1 < argc) {
                last_n = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--all") == 0) {
                show_all = true;
            } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search") == 0) && i + 1 < argc) {
                search_kw = argv[++i];
            } else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--stats") == 0) {
                show_stats = true;
            } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--clear") == 0) {
                do_clear = true;
            } else {
                fprintf(stderr, "Unknown or incomplete option '%s'. Use --help for usage.\n", argv[i]);
                return 1;
            }
        }

        LogBook lb;
        logbook_init(&lb, filepath);

        if (do_clear) {
            if (logbook_clear(&lb)) {
                printf("[LOGBOOK] Successfully cleared log file '%s'.\n", lb.filepath);
            } else {
                fprintf(stderr, "[ERROR] Failed to clear log file.\n");
                return 1;
            }
        }

        if (add_msg) {
            bool ok;
            if (tag) {
                ok = logbook_append_with_tag(&lb, tag, add_msg);
            } else {
                ok = logbook_append(&lb, add_msg);
            }
            if (ok) {
                printf("[LOGBOOK] Entry appended successfully.\n");
            } else {
                fprintf(stderr, "[ERROR] Failed to append entry.\n");
                return 1;
            }
        }

        if (last_n > 0) {
            logbook_display_last_n(&lb, last_n);
        }

        if (show_all) {
            logbook_display_all(&lb);
        }

        if (search_kw) {
            logbook_search(&lb, search_kw);
        }

        if (show_stats) {
            LogBookStats stats;
            if (logbook_get_stats(&lb, &stats)) {
                printf("\n================================================================\n");
                printf("                     LOGBOOK STATISTICS                         \n");
                printf("================================================================\n");
                printf(" Log File:         %s\n", lb.filepath);
                printf(" Total Entries:    %d\n", stats.total_entries);
                printf(" File Size:        %lld bytes (%.2f KB)\n", stats.file_size_bytes, stats.file_size_bytes / 1024.0);
                printf(" First Entry Date: %s\n", stats.first_timestamp);
                printf(" Last Entry Date:  %s\n", stats.last_timestamp);
                printf("================================================================\n\n");
            }
        }

        return 0;
    }

    /* Interactive Menu Mode */
    print_banner();
    LogBook lb;
    logbook_init(&lb, filepath);

    char input_buf[MAX_LOG_LINE_LEN];

    while (1) {
        print_menu(lb.filepath);
        if (!fgets(input_buf, sizeof(input_buf), stdin)) {
            break;
        }

        int choice = -1;
        if (sscanf(input_buf, "%d", &choice) != 1) {
            printf("[!] Invalid input. Please enter a number between 0 and 8.\n");
            continue;
        }

        if (choice == 0) {
            printf("\nExiting Logbook. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1: {
                printf("\nEnter log message: ");
                read_line_stdin(input_buf, sizeof(input_buf));
                if (input_buf[0] == '\0') {
                    printf("[!] Cannot append empty log entry.\n");
                } else {
                    if (logbook_append(&lb, input_buf)) {
                        printf("[✓] Log entry appended with timestamp successfully!\n");
                    }
                }
                break;
            }
            case 2: {
                char tag_buf[64];
                printf("\nEnter tag/level (e.g. INFO, WARN, ERROR, DEBUG): ");
                read_line_stdin(tag_buf, sizeof(tag_buf));
                printf("Enter log message: ");
                read_line_stdin(input_buf, sizeof(input_buf));
                if (input_buf[0] == '\0') {
                    printf("[!] Cannot append empty log entry.\n");
                } else {
                    if (logbook_append_with_tag(&lb, tag_buf, input_buf)) {
                        printf("[✓] Tagged log entry appended successfully!\n");
                    }
                }
                break;
            }
            case 3: {
                printf("\nEnter number of recent entries to display (N): ");
                read_line_stdin(input_buf, sizeof(input_buf));
                int n = atoi(input_buf);
                if (n <= 0) {
                    printf("[!] N must be a positive integer (>= 1).\n");
                } else {
                    logbook_display_last_n(&lb, n);
                }
                break;
            }
            case 4: {
                logbook_display_all(&lb);
                break;
            }
            case 5: {
                printf("\nEnter search keyword or date: ");
                read_line_stdin(input_buf, sizeof(input_buf));
                if (input_buf[0] == '\0') {
                    printf("[!] Keyword cannot be empty.\n");
                } else {
                    logbook_search(&lb, input_buf);
                }
                break;
            }
            case 6: {
                LogBookStats stats;
                if (logbook_get_stats(&lb, &stats)) {
                    printf("\n================================================================\n");
                    printf("                     LOGBOOK STATISTICS                         \n");
                    printf("================================================================\n");
                    printf(" Log File:         %s\n", lb.filepath);
                    printf(" Total Entries:    %d\n", stats.total_entries);
                    printf(" File Size:        %lld bytes (%.2f KB)\n", stats.file_size_bytes, stats.file_size_bytes / 1024.0);
                    printf(" First Entry Date: %s\n", stats.first_timestamp);
                    printf(" Last Entry Date:  %s\n", stats.last_timestamp);
                    printf("================================================================\n");
                }
                break;
            }
            case 7: {
                printf("\nAre you sure you want to clear '%s'? This cannot be undone (y/N): ", lb.filepath);
                read_line_stdin(input_buf, sizeof(input_buf));
                if (input_buf[0] == 'y' || input_buf[0] == 'Y') {
                    if (logbook_clear(&lb)) {
                        printf("[✓] Logbook file cleared successfully.\n");
                    }
                } else {
                    printf("[-] Clear operation cancelled.\n");
                }
                break;
            }
            case 8: {
                printf("\nEnter new target log file path: ");
                read_line_stdin(input_buf, sizeof(input_buf));
                if (input_buf[0] != '\0') {
                    logbook_init(&lb, input_buf);
                    printf("[✓] Active log file set to: %s\n", lb.filepath);
                }
                break;
            }
            default:
                printf("[!] Unrecognized option %d. Please select 0 to 8.\n", choice);
                break;
        }
    }

    return 0;
}

