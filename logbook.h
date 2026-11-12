/**
 * @file logbook.h
 * @brief Core header file for the Logbook Application.
 *
 * Provides APIs for appending timestamped log entries to a file,
 * retrieving and displaying the last N entries, searching, and managing logs.
 *
 * Course: Programming Project
 * Topic: File I/O & Time Functions
 */

#ifndef LOGBOOK_H
#define LOGBOOK_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_LOG_FILENAME "logbook.txt"
#define LOG_TIMESTAMP_LEN    32
#define MAX_LOG_LINE_LEN     4096

/**
 * @brief Structure representing a Logbook handle.
 */
typedef struct {
    char filepath[512];
} LogBook;

/**
 * @brief Structure representing statistics about a log file.
 */
typedef struct {
    int total_entries;
    long long file_size_bytes;
    char first_timestamp[LOG_TIMESTAMP_LEN];
    char last_timestamp[LOG_TIMESTAMP_LEN];
} LogBookStats;

/* ========================================================================= */
/* LIFECYCLE MANAGEMENT                                                      */
/* ========================================================================= */

/**
 * @brief Creates a LogBook handle for the given file path.
 * @param filepath Path to the log file (or NULL to use DEFAULT_LOG_FILENAME).
 * @return Pointer to allocated LogBook handle, or NULL on memory allocation failure.
 */
LogBook* logbook_create(const char *filepath);

/**
 * @brief Initializes a stack-allocated LogBook structure.
 * @param lb Pointer to LogBook structure.
 * @param filepath Path to the log file (or NULL for default).
 */
void logbook_init(LogBook *lb, const char *filepath);

/**
 * @brief Destroys and frees a dynamically allocated LogBook handle.
 * @param lb Pointer to LogBook handle.
 */
void logbook_destroy(LogBook *lb);

/* ========================================================================= */
/* CORE OPERATIONS                                                           */
/* ========================================================================= */

/**
 * @brief Appends a timestamped log entry to the logbook file.
 *
 * Automatically generates a timestamp in the format "[YYYY-MM-DD HH:MM:SS]"
 * using standard C time functions and appends the message.
 *
 * @param lb Pointer to LogBook instance.
 * @param message The message text to append.
 * @return true if successfully written and flushed to disk, false otherwise.
 */
bool logbook_append(LogBook *lb, const char *message);

/**
 * @brief Appends a timestamped log entry with an optional tag/level (e.g., INFO, WARN, ERROR).
 *
 * Format: "[YYYY-MM-DD HH:MM:SS] [TAG] Message"
 *
 * @param lb Pointer to LogBook instance.
 * @param tag Tag or log level (e.g., "INFO", "ERROR", "NOTE").
 * @param message The message text.
 * @return true on success, false on failure.
 */
bool logbook_append_with_tag(LogBook *lb, const char *tag, const char *message);

/* ========================================================================= */
/* RETRIEVAL AND DISPLAY                                                     */
/* ========================================================================= */

/**
 * @brief Counts the total number of entries currently stored in the log file.
 * @param lb Pointer to LogBook instance.
 * @return Total number of entries (>= 0), or -1 if the file cannot be accessed.
 */
int logbook_count_entries(LogBook *lb);

/**
 * @brief Retrieves the last N entries as an array of dynamically allocated strings.
 *
 * Uses an efficient ring-buffer algorithm requiring only O(N) memory
 * and a single sequential pass over the log file.
 *
 * @param lb Pointer to LogBook instance.
 * @param n Number of recent entries requested (must be > 0).
 * @param out_entries Output pointer to array of allocated string pointers.
 * @return The actual number of entries retrieved (0 <= count <= n).
 */
int logbook_get_last_n(LogBook *lb, int n, char ***out_entries);

/**
 * @brief Frees the array of strings returned by logbook_get_last_n or search.
 * @param entries Pointer array of strings.
 * @param count Number of elements in the array.
 */
void logbook_free_entries(char **entries, int count);

/**
 * @brief Displays the last N entries to standard output in a clean, formatted view.
 * @param lb Pointer to LogBook instance.
 * @param n Number of entries to display.
 * @return Number of entries displayed.
 */
int logbook_display_last_n(LogBook *lb, int n);

/**
 * @brief Displays all entries currently stored in the log file.
 * @param lb Pointer to LogBook instance.
 * @return Number of entries displayed.
 */
int logbook_display_all(LogBook *lb);

/**
 * @brief Searches for entries containing a specified keyword/substring (case-insensitive).
 * @param lb Pointer to LogBook instance.
 * @param keyword Substring to search for.
 * @return Number of matches found and displayed.
 */
int logbook_search(LogBook *lb, const char *keyword);

/* ========================================================================= */
/* UTILITY AND MAINTENANCE                                                   */
/* ========================================================================= */

/**
 * @brief Clears (truncates) the log file.
 * @param lb Pointer to LogBook instance.
 * @return true on success, false otherwise.
 */
bool logbook_clear(LogBook *lb);

/**
 * @brief Computes statistics for the logbook.
 * @param lb Pointer to LogBook instance.
 * @param out_stats Pointer to LogBookStats structure to receive the values.
 * @return true on success, false otherwise.
 */
bool logbook_get_stats(LogBook *lb, LogBookStats *out_stats);

/**
 * @brief Generates the current system timestamp formatted as "YYYY-MM-DD HH:MM:SS".
 * @param buffer Output buffer.
 * @param buf_size Size of the output buffer.
 */
void logbook_get_timestamp(char *buffer, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* LOGBOOK_H */

