/**
 * @file logbook.c
 * @brief Implementation of the Logbook Application core functions.
 *
 * Implements file I/O operations, timestamp generation, circular buffer
 * retrieval for last N entries, search, and statistical computations.
 */

#include "logbook.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

/* Helper duplicate string function (ANSI C compliant) */
static char* logbook_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *dst = (char*)malloc(len + 1);
    if (dst) {
        memcpy(dst, src, len + 1);
    }
    return dst;
}

/* Helper to trim trailing carriage returns and newlines in-place */
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
            if (*n == '\0') {
                return haystack;
            }
        }
    }
    return NULL;
}

/* ========================================================================= */
/* LIFECYCLE MANAGEMENT                                                      */
/* ========================================================================= */

LogBook* logbook_create(const char *filepath) {
    LogBook *lb = (LogBook*)malloc(sizeof(LogBook));
    if (!lb) {
        fprintf(stderr, "[ERROR] Memory allocation failed for LogBook instance.\n");
        return NULL;
    }
    logbook_init(lb, filepath);
    return lb;
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

void logbook_destroy(LogBook *lb) {
    if (lb) {
        free(lb);
    }
}

/* ========================================================================= */
/* TIME UTILITY                                                              */
/* ========================================================================= */

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

/* ========================================================================= */
/* CORE OPERATIONS                                                           */
/* ========================================================================= */

bool logbook_append(LogBook *lb, const char *message) {
    if (!lb || !message) return false;

    char timestamp[LOG_TIMESTAMP_LEN];
    logbook_get_timestamp(timestamp, sizeof(timestamp));

    FILE *fp = fopen(lb->filepath, "a");
    if (!fp) {
        fprintf(stderr, "[ERROR] Failed to open log file '%s' for appending.\n", lb->filepath);
        return false;
    }

    /* Print entry with timestamp */
    int written = fprintf(fp, "[%s] %s\n", timestamp, message);
    if (written < 0) {
        fprintf(stderr, "[ERROR] Failed to write entry to '%s'.\n", lb->filepath);
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
        fprintf(stderr, "[ERROR] Failed to open log file '%s' for appending.\n", lb->filepath);
        return false;
    }

    int written;
    if (tag && tag[0] != '\0') {
        written = fprintf(fp, "[%s] [%s] %s\n", timestamp, tag, message);
    } else {
        written = fprintf(fp, "[%s] %s\n", timestamp, message);
    }

    if (written < 0) {
        fprintf(stderr, "[ERROR] Failed to write entry to '%s'.\n", lb->filepath);
        fclose(fp);
        return false;
    }

    fflush(fp);
    fclose(fp);
    return true;
}

/* ========================================================================= */
/* RETRIEVAL AND DISPLAY                                                     */
/* ========================================================================= */

int logbook_count_entries(LogBook *lb) {
    if (!lb) return -1;

    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) {
        return 0; /* File does not exist yet or empty */
    }

    int count = 0;
    char buffer[MAX_LOG_LINE_LEN];

    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_trailing_newlines(buffer);
        if (buffer[0] != '\0') {
            count++;
        }
    }

    fclose(fp);
    return count;
}

int logbook_get_last_n(LogBook *lb, int n, char ***out_entries) {
    if (!lb || !out_entries) return 0;
    *out_entries = NULL;

    if (n <= 0) return 0;

    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) {
        return 0; /* Log file does not exist yet */
    }

    /* Allocate ring buffer of capacity n */
    char **ring = (char**)calloc((size_t)n, sizeof(char*));
    if (!ring) {
        fprintf(stderr, "[ERROR] Out of memory allocating ring buffer for %d entries.\n", n);
        fclose(fp);
        return 0;
    }

    int total_read = 0;
    int head = 0;
    char buffer[MAX_LOG_LINE_LEN];

    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_trailing_newlines(buffer);
        if (buffer[0] == '\0') {
            continue; /* Skip purely blank lines */
        }

        if (total_read < n) {
            ring[total_read] = logbook_strdup(buffer);
            total_read++;
        } else {
            /* Evict oldest entry in ring buffer */
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

    /* Assemble chronologically ordered array */
    char **result = (char**)malloc((size_t)count * sizeof(char*));
    if (!result) {
        for (int i = 0; i < (total_read < n ? total_read : n); i++) {
            free(ring[i]);
        }
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
        if (entries[i]) {
            free(entries[i]);
        }
    }
    free(entries);
}

int logbook_display_last_n(LogBook *lb, int n) {
    if (!lb) return 0;

    if (n <= 0) {
        printf("[LOGBOOK] Invalid count N=%d. Please provide N >= 1.\n", n);
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

    if (count == 0) {
        printf("  (Log file is empty)\n");
    }
    printf("================================================================\n");
    printf(" Total entries displayed: %d\n\n", count);

    fclose(fp);
    return count;
}

int logbook_search(LogBook *lb, const char *keyword) {
    if (!lb || !keyword || keyword[0] == '\0') {
        printf("[LOGBOOK] Search keyword cannot be empty.\n");
        return 0;
    }

    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) {
        printf("[LOGBOOK] No log file '%s' found.\n", lb->filepath);
        return 0;
    }

    char buffer[MAX_LOG_LINE_LEN];
    int line_num = 0;
    int match_count = 0;

    printf("\n================================================================\n");
    printf(" SEARCH RESULTS FOR: \"%s\"\n", keyword);
    printf("================================================================\n");

    while (fgets(buffer, sizeof(buffer), fp)) {
        line_num++;
        trim_trailing_newlines(buffer);
        if (str_istr(buffer, keyword)) {
            match_count++;
            printf("Line %4d: %s\n", line_num, buffer);
        }
    }

    fclose(fp);

    if (match_count == 0) {
        printf(" No entries matched \"%s\".\n", keyword);
    }
    printf("================================================================\n");
    printf(" Found %d match(es) across %d lines.\n\n", match_count, line_num);

    return match_count;
}

/* ========================================================================= */
/* UTILITY AND MAINTENANCE                                                   */
/* ========================================================================= */

bool logbook_clear(LogBook *lb) {
    if (!lb) return false;

    FILE *fp = fopen(lb->filepath, "w");
    if (!fp) {
        fprintf(stderr, "[ERROR] Unable to truncate/clear log file '%s'.\n", lb->filepath);
        return false;
    }

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
    } else {
        out_stats->file_size_bytes = 0;
    }

    FILE *fp = fopen(lb->filepath, "r");
    if (!fp) {
        return true; /* File doesn't exist yet, stats remain 0 / N/A */
    }

    char buffer[MAX_LOG_LINE_LEN];
    int count = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        trim_trailing_newlines(buffer);
        if (buffer[0] == '\0') continue;

        count++;

        /* Extract timestamp if line starts with '[' */
        if (buffer[0] == '[') {
            char ts[LOG_TIMESTAMP_LEN] = {0};
            char *close_bracket = strchr(buffer, ']');
            if (close_bracket) {
                size_t ts_len = (size_t)(close_bracket - (buffer + 1));
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

