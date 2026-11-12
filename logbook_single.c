/**
 * @file logbook_single.c
 * @brief Standalone, self-contained single-file C implementation of the Logbook Application.
 *
 * Designed for single-file submission requirements in C programming courses.
 * Contains the complete LogBook API, Ring-Buffer Last-N retrieval,
 * time-formatting utilities, and interactive CLI menu.
 *
 * Topic: File I/O & Time Functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>

#define DEFAULT_LOG_FILENAME "logbook.txt"
#define LOG_TIMESTAMP_LEN    32
#define MAX_LOG_LINE_LEN     4096

typedef struct {
    char filepath[512];
} LogBook;

typedef struct {
    int total_entries;
    long long file_size_bytes;
    char first_timestamp[LOG_TIMESTAMP_LEN];
    char last_timestamp[LOG_TIMESTAMP_LEN];
} LogBookStats;

/* Helper duplicate string function */
static char* logbook_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *dst = (char*)malloc(len + 1);
    if (dst) {
        memcpy(dst, src, len + 1);
    }
    return dst;
}

/* Helper to strip trailing newlines and carriage returns */
static void trim_trailing_newlines(char *str) {
    if (!str) return;
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

/* Helper for case-insensitive substring search */
static const char* str_istr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    if (*needle == '\0') return haystack;

    for (; *haystack; ++haystack) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            const char *h = haystack;
            const char *n = needle;
            while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
                ++h;
                ++n;
            }
            if (*n == '\0') return haystack;
        }
    }
    return NULL;
}

/* Generate current timestamp: YYYY-MM-DD HH:MM:SS */
void logbook_get_timestamp(char *buffer, size_t buf_size) {
    if (!buffer || buf_size == 0) return;
    time_t raw_time = time(NULL);
    struct tm *tm_info = localtime(&raw_time);
    if (tm_info) {
        strftime(buffer, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(buffer, buf_size, "1970-01-01 00:00:00");
    }
}

void logbook_init(LogBook *lb, const char *filepath) {
    if (!lb) return;
    if (filepath && filepath[0] != '\0') {
        strncpy(lb->filepath, filepath, sizeof(lb->filepath) - 1);
        lb->filepath[sizeof(lb->filepath) - 1] = '\0';
    } else {
        strncpy(lb->filepath, DEFAULT_LOG_FILENAME, sizeof(lb->filepath) - 1);
        lb->filepath[sizeof(lb->filepath) - 1] = '\0';
    }
}

bool logbook_append(LogBook *lb, const char *message) {
    if (!lb || !message) return false;

    char timestamp[LOG_TIMESTAMP_LEN];
    logbook_get_timestamp(timestamp, sizeof(timestamp));

    FILE *fp = fopen(lb->filepath, "a");
    if (!fp) {
        fprintf(stderr, "[ERROR] Cannot open file '%s' in append mode.\n", lb->filepath);
        return false;
    }

    int written = fprintf(fp, "[%s] %s\n", timestamp, message);
    if (written < 0) {
        fclose(fp);
        return false;
    }

    fflush(fp);
    fclose(fp);
    return true;
}

bool logbook_append_with_tag(LogBook *lb, const char *tag, const char *message) {
    if (!lb || !message) return false;

    char timestamp[LOG_TIMESTAMP_LEN];
    logbook_get_timestamp(timestamp, sizeof(timestamp));

    FILE *fp = fopen(lb->filepath, "a");
    if (!fp) {
        fprintf(stderr, "[ERROR] Cannot open file '%s' in append mode.\n", lb->filepath);
        return false;
    }

    int written;
    if (tag && tag[0] != '\0') {
        written = fprintf(fp, "[%s] [%s] %s\n", timestamp, tag, message);
    } else {
        written = fprintf(fp, "[%s] %s\n", timestamp, message);
    }

    if (written < 0) {
        fclose(fp);
        return false;
    }

    fflush(fp);
    fclose(fp);
    return true;
}

int logbook_count_entries(LogBook *lb) {
    if (!lb) return -1;
    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) return 0;

    int count = 0;
    char buffer[MAX_LOG_LINE_LEN];
    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_trailing_newlines(buffer);
        if (buffer[0] != '\0') count++;
    }
    fclose(fp);
    return count;
}

int logbook_get_last_n(LogBook *lb, int n, char ***out_entries) {
    if (!lb || !out_entries) return 0;
    *out_entries = NULL;
    if (n <= 0) return 0;

    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) return 0;

    char **ring = (char**)calloc((size_t)n, sizeof(char*));
    if (!ring) {
        fclose(fp);
        return 0;
    }

    int total_read = 0;
    int head = 0;
    char buffer[MAX_LOG_LINE_LEN];

    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_trailing_newlines(buffer);
        if (buffer[0] == '\0') continue;

        if (total_read < n) {
            ring[total_read] = logbook_strdup(buffer);
            total_read++;
        } else {
            free(ring[head]);
            ring[head] = logbook_strdup(buffer);
            head = (head + 1) % n;
            total_read++;
        }
    }
    fclose(fp);

    int count = (total_read < n) ? total_read : n;
    if (count == 0) {
        free(ring);
        return 0;
    }

    char **result = (char**)malloc((size_t)count * sizeof(char*));
    if (!result) {
        for (int i = 0; i < count; i++) free(ring[i]);
        free(ring);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        int idx = (total_read < n) ? i : (head + i) % n;
        result[i] = ring[idx];
    }
    free(ring);

    *out_entries = result;
    return count;
}

void logbook_free_entries(char **entries, int count) {
    if (!entries) return;
    for (int i = 0; i < count; i++) {
        if (entries[i]) free(entries[i]);
    }
    free(entries);
}

int logbook_display_last_n(LogBook *lb, int n) {
    if (!lb || n <= 0) {
        printf("[LOGBOOK] Invalid N parameter.\n");
        return 0;
    }

    char **entries = NULL;
    int count = logbook_get_last_n(lb, n, &entries);

    if (count == 0) {
        printf("\n+--------------------------------------------------------------+\n");
        printf("| [LOGBOOK] Log file is currently empty or does not exist.     |\n");
        printf("+--------------------------------------------------------------+\n\n");
        return 0;
    }

    int total = logbook_count_entries(lb);

    printf("\n================================================================\n");
    printf("                  LAST %d LOGBOOK ENTRIES                      \n", count);
    if (total > count) {
        printf("       (Showing %d of %d total entries in chronological order)  \n", count, total);
    } else {
        printf("                (Showing all %d available entries)              \n", count);
    }
    printf("================================================================\n");

    for (int i = 0; i < count; i++) {
        printf("[%3d] %s\n", i + 1, entries[i]);
    }
    printf("================================================================\n\n");

    logbook_free_entries(entries, count);
    return count;
}

int logbook_display_all(LogBook *lb) {
    if (!lb) return 0;
    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) {
        printf("\n[LOGBOOK] No log file '%s' found or logbook is empty.\n\n", lb->filepath);
        return 0;
    }

    char buffer[MAX_LOG_LINE_LEN];
    int count = 0;
    printf("\n================================================================\n");
    printf("                   ALL LOGBOOK ENTRIES (%s)\n", lb->filepath);
    printf("================================================================\n");

    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_trailing_newlines(buffer);
        if (buffer[0] != '\0') {
            count++;
            printf("[%4d] %s\n", count, buffer);
        }
    }
    printf("================================================================\n");
    printf(" Total entries displayed: %d\n\n", count);

    fclose(fp);
    return count;
}

int logbook_search(LogBook *lb, const char *keyword) {
    if (!lb || !keyword || keyword[0] == '\0') {
        printf("[LOGBOOK] Keyword cannot be empty.\n");
        return 0;
    }

    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) {
        printf("[LOGBOOK] No log file '%s' found.\n", lb->filepath);
        return 0;
    }

    char buffer[MAX_LOG_LINE_LEN];
    int line_num = 0, matches = 0;

    printf("\n================================================================\n");
    printf(" SEARCH RESULTS FOR: \"%s\"\n", keyword);
    printf("================================================================\n");

    while (fgets(buffer, sizeof(buffer), fp)) {
        line_num++;
        trim_trailing_newlines(buffer);
        if (str_istr(buffer, keyword)) {
            matches++;
            printf("Line %4d: %s\n", line_num, buffer);
        }
    }
    fclose(fp);
    printf("================================================================\n");
    printf(" Found %d match(es) across %d lines.\n\n", matches, line_num);
    return matches;
}

bool logbook_clear(LogBook *lb) {
    if (!lb) return false;
    FILE *fp = fopen(lb->filepath, "w");
    if (!fp) return false;
    fclose(fp);
    return true;
}

bool logbook_get_stats(LogBook *lb, LogBookStats *out_stats) {
    if (!lb || !out_stats) return false;
    memset(out_stats, 0, sizeof(LogBookStats));
    strncpy(out_stats->first_timestamp, "N/A", sizeof(out_stats->first_timestamp) - 1);
    strncpy(out_stats->last_timestamp, "N/A", sizeof(out_stats->last_timestamp) - 1);

    struct stat st;
    if (stat(lb->filepath, &st) == 0) {
        out_stats->file_size_bytes = (long long)st.st_size;
    }

    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) return true;

    char buffer[MAX_LOG_LINE_LEN];
    int count = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_trailing_newlines(buffer);
        if (buffer[0] == '\0') continue;
        count++;

        if (buffer[0] == '[') {
            char ts[LOG_TIMESTAMP_LEN] = {0};
            char *close_brk = strchr(buffer, ']');
            if (close_brk) {
                size_t ts_len = (size_t)(close_brk - (buffer + 1));
                if (ts_len < sizeof(ts)) {
                    strncpy(ts, buffer + 1, ts_len);
                    ts[ts_len] = '\0';
                }
            }
            if (ts[0] != '\0') {
                if (count == 1) {
                    strncpy(out_stats->first_timestamp, ts, sizeof(out_stats->first_timestamp) - 1);
                }
                strncpy(out_stats->last_timestamp, ts, sizeof(out_stats->last_timestamp) - 1);
            }
        }
    }
    fclose(fp);
    out_stats->total_entries = count;
    return true;
}

/* CLI and Interactive Menu Entry Point */
int main(int argc, char *argv[]) {
    LogBook lb;
    logbook_init(&lb, DEFAULT_LOG_FILENAME);

    if (argc > 1) {
        if (strcmp(argv[1], "--add") == 0 && argc > 2) {
            logbook_append(&lb, argv[2]);
            printf("[LOGBOOK] Entry appended successfully.\n");
            return 0;
        } else if (strcmp(argv[1], "--last") == 0 && argc > 2) {
            int n = atoi(argv[2]);
            logbook_display_last_n(&lb, n);
            return 0;
        } else if (strcmp(argv[1], "--all") == 0) {
            logbook_display_all(&lb);
            return 0;
        } else if (strcmp(argv[1], "--clear") == 0) {
            logbook_clear(&lb);
            printf("[LOGBOOK] Logbook cleared.\n");
            return 0;
        } else {
            printf("Usage: %s [--add \"msg\"] [--last N] [--all] [--clear]\n", argv[0]);
            return 0;
        }
    }

    /* Interactive Menu */
    printf("================================================================\n");
    printf("          LOGBOOK APPLICATION (File I/O & Time Functions)       \n");
    printf("================================================================\n");

    char input[MAX_LOG_LINE_LEN];
    while (1) {
        printf("\n--- LOGBOOK MENU (%s) ---\n", lb.filepath);
        printf("  [1] Append New Timestamped Entry\n");
        printf("  [2] Append Entry with Tag (INFO, ERROR, etc.)\n");
        printf("  [3] Display Last N Entries\n");
        printf("  [4] Display All Entries\n");
        printf("  [5] Search Entries by Keyword\n");
        printf("  [6] View Logbook Statistics\n");
        printf("  [7] Clear Logbook\n");
        printf("  [0] Exit\n");
        printf("Enter choice (0-7): ");

        if (!fgets(input, sizeof(input), stdin)) break;
        int choice = -1;
        if (sscanf(input, "%d", &choice) != 1) continue;

        if (choice == 0) {
            printf("Exiting. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1:
                printf("Enter message: ");
                if (fgets(input, sizeof(input), stdin)) {
                    trim_trailing_newlines(input);
                    if (input[0] != '\0') {
                        logbook_append(&lb, input);
                        printf("[✓] Timestamped entry recorded.\n");
                    }
                }
                break;
            case 2: {
                char tag[64];
                printf("Enter tag: ");
                if (fgets(tag, sizeof(tag), stdin)) trim_trailing_newlines(tag);
                printf("Enter message: ");
                if (fgets(input, sizeof(input), stdin)) {
                    trim_trailing_newlines(input);
                    if (input[0] != '\0') {
                        logbook_append_with_tag(&lb, tag, input);
                        printf("[✓] Tagged entry recorded.\n");
                    }
                }
                break;
            }
            case 3:
                printf("Enter N: ");
                if (fgets(input, sizeof(input), stdin)) {
                    int n = atoi(input);
                    if (n > 0) logbook_display_last_n(&lb, n);
                    else printf("[!] Please enter a positive integer.\n");
                }
                break;
            case 4:
                logbook_display_all(&lb);
                break;
            case 5:
                printf("Enter keyword: ");
                if (fgets(input, sizeof(input), stdin)) {
                    trim_trailing_newlines(input);
                    if (input[0] != '\0') logbook_search(&lb, input);
                }
                break;
            case 6: {
                LogBookStats stats;
                if (logbook_get_stats(&lb, &stats)) {
                    printf("\n--- Statistics for %s ---\n", lb.filepath);
                    printf(" Total Entries:    %d\n", stats.total_entries);
                    printf(" File Size:        %lld bytes\n", stats.file_size_bytes);
                    printf(" First Entry Date: %s\n", stats.first_timestamp);
                    printf(" Last Entry Date:  %s\n", stats.last_timestamp);
                }
                break;
            }
            case 7:
                printf("Are you sure? (y/N): ");
                if (fgets(input, sizeof(input), stdin) && (input[0] == 'y' || input[0] == 'Y')) {
                    logbook_clear(&lb);
                    printf("[✓] Logbook cleared.\n");
                }
                break;
            default:
                printf("[!] Invalid selection.\n");
                break;
        }
    }
    return 0;
}

