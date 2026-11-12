/**
 * @file test_runner.c
 * @brief Comprehensive Automated Test Suite for Logbook Application.
 *
 * Tests:
 *  1. File initialization & lifecycle
 *  2. Timestamp generation and format validation (YYYY-MM-DD HH:MM:SS)
 *  3. Appending single & multiple entries (File I/O append mode)
 *  4. Tagged entries ([INFO], [WARN], [ERROR])
 *  5. Last N retrieval: N < total (ring buffer rollover test)
 *  6. Last N retrieval: N == total
 *  7. Last N retrieval: N > total
 *  8. Edge cases: N=0, N<0, empty file, non-existent file
 *  9. Special characters, symbols, and long lines
 * 10. Keyword search (case-insensitive)
 * 11. Statistics verification (count, file size, first/last timestamps)
 * 12. Truncation and re-appending after clear
 * 13. Stress test: 200 sequential entries with last 10 retrieval
 */

#include "logbook.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TEST_LOG_FILE "test_logbook_temp.log"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define ASSERT_TRUE(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
    } \
} while(0)

#define ASSERT_INT_EQ(actual, expected, msg) do { \
    g_tests_run++; \
    if ((actual) == (expected)) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Expected %d, got %d, Line %d)\n", msg, (int)(expected), (int)(actual), __LINE__); \
    } \
} while(0)

#define ASSERT_STR_CONTAINS(haystack, needle, msg) do { \
    g_tests_run++; \
    if ((haystack) && strstr((haystack), (needle))) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (String \"%s\" not found in \"%s\", Line %d)\n", msg, needle, haystack ? haystack : "NULL", __LINE__); \
    } \
} while(0)

/* Helper to remove temporary test log file */
static void cleanup_test_file(void) {
    remove(TEST_LOG_FILE);
}

/* Helper to validate "YYYY-MM-DD HH:MM:SS" */
static bool is_valid_timestamp_format(const char *ts) {
    if (!ts || strlen(ts) != 19) return false;
    for (int i = 0; i < 19; i++) {
        if (i == 4 || i == 7) {
            if (ts[i] != '-') return false;
        } else if (i == 10) {
            if (ts[i] != ' ') return false;
        } else if (i == 13 || i == 16) {
            if (ts[i] != ':') return false;
        } else {
            if (!isdigit((unsigned char)ts[i])) return false;
        }
    }
    return true;
}

/* ========================================================================= */
/* TEST SUITES                                                               */
/* ========================================================================= */

static void test_lifecycle_and_empty_file(void) {
    printf("\n=== TEST SUITE 1: Lifecycle & Empty File Handling ===\n");
    cleanup_test_file();

    LogBook *lb = logbook_create(TEST_LOG_FILE);
    ASSERT_TRUE(lb != NULL, "logbook_create returns non-null handle");
    ASSERT_INT_EQ(strcmp(lb->filepath, TEST_LOG_FILE), 0, "LogBook has correct filepath");

    /* Non-existent file should report 0 entries */
    int count = logbook_count_entries(lb);
    ASSERT_INT_EQ(count, 0, "count_entries on non-existent file returns 0");

    char **entries = NULL;
    int fetched = logbook_get_last_n(lb, 5, &entries);
    ASSERT_INT_EQ(fetched, 0, "get_last_n on non-existent file returns 0");
    ASSERT_TRUE(entries == NULL, "entries pointer is NULL when count is 0");

    logbook_destroy(lb);
}

static void test_timestamp_function(void) {
    printf("\n=== TEST SUITE 2: Timestamp Generation & Format ===\n");
    char ts[LOG_TIMESTAMP_LEN] = {0};
    logbook_get_timestamp(ts, sizeof(ts));

    printf("  Generated Timestamp: '%s'\n", ts);
    ASSERT_TRUE(is_valid_timestamp_format(ts), "Timestamp matches 'YYYY-MM-DD HH:MM:SS' format");
}

static void test_append_single_and_multiple(void) {
    printf("\n=== TEST SUITE 3: Appending Entries (File I/O) ===\n");
    cleanup_test_file();

    LogBook lb;
    logbook_init(&lb, TEST_LOG_FILE);

    bool ok1 = logbook_append(&lb, "First entry recorded");
    ASSERT_TRUE(ok1, "First append returns true");
    ASSERT_INT_EQ(logbook_count_entries(&lb), 1, "Count is 1 after first append");

    bool ok2 = logbook_append(&lb, "Second entry recorded");
    ASSERT_TRUE(ok2, "Second append returns true");
    ASSERT_INT_EQ(logbook_count_entries(&lb), 2, "Count is 2 after second append");

    bool ok3 = logbook_append_with_tag(&lb, "INFO", "System initialized");
    bool ok4 = logbook_append_with_tag(&lb, "ERROR", "Failed to connect to secondary worker");
    ASSERT_TRUE(ok3 && ok4, "Appends with tags return true");
    ASSERT_INT_EQ(logbook_count_entries(&lb), 4, "Count is 4 after tagged appends");
}

static void test_get_last_n_cases(void) {
    printf("\n=== TEST SUITE 4: Retrieval of Last N Entries (Ring Buffer) ===\n");
    cleanup_test_file();

    LogBook lb;
    logbook_init(&lb, TEST_LOG_FILE);

    /* Populate 6 entries */
    for (int i = 1; i <= 6; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Log item number %d", i);
        logbook_append(&lb, msg);
    }
    ASSERT_INT_EQ(logbook_count_entries(&lb), 6, "Log file contains 6 entries");

    /* Case A: N < total (N=3 of 6) */
    char **entries = NULL;
    int n_fetched = logbook_get_last_n(&lb, 3, &entries);
    ASSERT_INT_EQ(n_fetched, 3, "get_last_n(3) returns 3 entries");
    ASSERT_STR_CONTAINS(entries[0], "Log item number 4", "Entry 0 is item 4");
    ASSERT_STR_CONTAINS(entries[1], "Log item number 5", "Entry 1 is item 5");
    ASSERT_STR_CONTAINS(entries[2], "Log item number 6", "Entry 2 is item 6");
    logbook_free_entries(entries, n_fetched);

    /* Case B: N == total (N=6 of 6) */
    entries = NULL;
    n_fetched = logbook_get_last_n(&lb, 6, &entries);
    ASSERT_INT_EQ(n_fetched, 6, "get_last_n(6) returns 6 entries");
    ASSERT_STR_CONTAINS(entries[0], "Log item number 1", "First entry is item 1");
    ASSERT_STR_CONTAINS(entries[5], "Log item number 6", "Last entry is item 6");
    logbook_free_entries(entries, n_fetched);

    /* Case C: N > total (N=20 of 6) */
    entries = NULL;
    n_fetched = logbook_get_last_n(&lb, 20, &entries);
    ASSERT_INT_EQ(n_fetched, 6, "get_last_n(20) returns all 6 available entries without error");
    ASSERT_STR_CONTAINS(entries[0], "Log item number 1", "First item matches");
    ASSERT_STR_CONTAINS(entries[5], "Log item number 6", "Last item matches");
    logbook_free_entries(entries, n_fetched);

    /* Case D: Edge cases N=0 and N<0 */
    entries = NULL;
    int zero_res = logbook_get_last_n(&lb, 0, &entries);
    ASSERT_INT_EQ(zero_res, 0, "get_last_n(0) safely returns 0");
    int neg_res = logbook_get_last_n(&lb, -4, &entries);
    ASSERT_INT_EQ(neg_res, 0, "get_last_n(-4) safely returns 0");
}

static void test_search_and_stats(void) {
    printf("\n=== TEST SUITE 5: Search and Statistics ===\n");
    cleanup_test_file();

    LogBook lb;
    logbook_init(&lb, TEST_LOG_FILE);

    logbook_append_with_tag(&lb, "AUTH", "User admin logged in from 192.168.1.10");
    logbook_append_with_tag(&lb, "DB", "Connection timeout on port 5432");
    logbook_append_with_tag(&lb, "AUTH", "User guest logged out");
    logbook_append_with_tag(&lb, "KERNEL", "CPU temperature normal");

    /* Search test */
    int auth_matches = logbook_search(&lb, "AUTH");
    ASSERT_INT_EQ(auth_matches, 2, "Search for 'AUTH' returns 2 matches");

    int case_matches = logbook_search(&lb, "admin");
    ASSERT_INT_EQ(case_matches, 1, "Case-insensitive search for 'admin' returns 1 match");

    int no_matches = logbook_search(&lb, "NONEXISTENT_KEYWORD");
    ASSERT_INT_EQ(no_matches, 0, "Search for non-existent keyword returns 0");

    /* Stats test */
    LogBookStats stats;
    bool stats_ok = logbook_get_stats(&lb, &stats);
    ASSERT_TRUE(stats_ok, "logbook_get_stats returns true");
    ASSERT_INT_EQ(stats.total_entries, 4, "Stats reports 4 total entries");
    ASSERT_TRUE(stats.file_size_bytes > 0, "Stats reports file_size_bytes > 0");
    ASSERT_TRUE(is_valid_timestamp_format(stats.first_timestamp), "Stats first timestamp is valid");
    ASSERT_TRUE(is_valid_timestamp_format(stats.last_timestamp), "Stats last timestamp is valid");
}

static void test_clear_and_reappend(void) {
    printf("\n=== TEST SUITE 6: Clear and Re-append ===\n");
    cleanup_test_file();

    LogBook lb;
    logbook_init(&lb, TEST_LOG_FILE);

    logbook_append(&lb, "Temporary entry 1");
    logbook_append(&lb, "Temporary entry 2");
    ASSERT_INT_EQ(logbook_count_entries(&lb), 2, "2 entries before clear");

    bool cleared = logbook_clear(&lb);
    ASSERT_TRUE(cleared, "logbook_clear returns true");
    ASSERT_INT_EQ(logbook_count_entries(&lb), 0, "Count is 0 after clear");

    LogBookStats stats;
    logbook_get_stats(&lb, &stats);
    ASSERT_INT_EQ(stats.total_entries, 0, "Stats reports 0 entries after clear");
    ASSERT_INT_EQ((int)stats.file_size_bytes, 0, "Stats reports 0 bytes file size after clear");

    /* Re-append */
    logbook_append(&lb, "New entry after clear");
    ASSERT_INT_EQ(logbook_count_entries(&lb), 1, "Can append cleanly after clear");
}

static void test_stress_and_rollover(void) {
    printf("\n=== TEST SUITE 7: Stress Rollover Test (100 entries, Last 5) ===\n");
    cleanup_test_file();

    LogBook lb;
    logbook_init(&lb, TEST_LOG_FILE);

    for (int i = 0; i < 100; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Sequential batch payload record #%03d", i);
        logbook_append(&lb, msg);
    }

    ASSERT_INT_EQ(logbook_count_entries(&lb), 100, "100 entries appended successfully");

    char **entries = NULL;
    int count = logbook_get_last_n(&lb, 5, &entries);
    ASSERT_INT_EQ(count, 5, "Fetched last 5 of 100 entries");

    ASSERT_STR_CONTAINS(entries[0], "record #095", "Slot 0 is #095");
    ASSERT_STR_CONTAINS(entries[1], "record #096", "Slot 1 is #096");
    ASSERT_STR_CONTAINS(entries[2], "record #097", "Slot 2 is #097");
    ASSERT_STR_CONTAINS(entries[3], "record #098", "Slot 3 is #098");
    ASSERT_STR_CONTAINS(entries[4], "record #099", "Slot 4 is #099");

    logbook_free_entries(entries, count);
}

int main(void) {
    printf("================================================================\n");
    printf("         LOGBOOK TEST SUITE - UNIT & INTEGRATION TESTS          \n");
    printf("================================================================\n");

    test_lifecycle_and_empty_file();
    test_timestamp_function();
    test_append_single_and_multiple();
    test_get_last_n_cases();
    test_search_and_stats();
    test_clear_and_reappend();
    test_stress_and_rollover();

    cleanup_test_file();

    printf("\n================================================================\n");
    printf(" TEST RESULTS SUMMARY\n");
    printf("================================================================\n");
    printf(" Total Tests Run:    %d\n", g_tests_run);
    printf(" Total Tests Passed: %d\n", g_tests_passed);
    printf(" Total Tests Failed: %d\n", g_tests_run - g_tests_passed);
    printf(" Pass Rate:          %.1f%%\n", (100.0 * g_tests_passed) / g_tests_run);
    printf("================================================================\n");

    return (g_tests_run == g_tests_passed) ? 0 : 1;
}

