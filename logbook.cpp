/**
 * @file logbook.cpp
 * @brief Modern C++ (C++17) implementation of the Logbook Application.
 *
 * Demonstrates:
 *  - RAII and Object-Oriented Design (class LogBook)
 *  - C++ standard file streams (std::ifstream, std::ofstream, std::ios::app)
 *  - Modern chrono & time formatting (std::chrono::system_clock, std::put_time)
 *  - STL containers (std::deque for ring buffer, std::vector, std::string)
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>

class LogBook {
private:
    std::string filepath_;

    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm_now, &now_c);
#else
        localtime_r(&now_c, &tm_now);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

public:
    explicit LogBook(std::string filepath = "logbook.txt")
        : filepath_(std::move(filepath)) {}

    const std::string& getFilePath() const { return filepath_; }
    void setFilePath(const std::string& path) { filepath_ = path; }

    bool append(const std::string& message, const std::string& tag = "") {
        std::ofstream outfile(filepath_, std::ios::app);
        if (!outfile.is_open()) {
            std::cerr << "[ERROR] Could not open file '" << filepath_ << "' in append mode.\n";
            return false;
        }

        std::string ts = getCurrentTimestamp();
        if (!tag.empty()) {
            outfile << "[" << ts << "] [" << tag << "] " << message << "\n";
        } else {
            outfile << "[" << ts << "] " << message << "\n";
        }

        outfile.flush();
        return true;
    }

    size_t countEntries() const {
        std::ifstream infile(filepath_);
        if (!infile.is_open()) return 0;

        size_t count = 0;
        std::string line;
        while (std::getline(infile, line)) {
            if (!line.empty() && line.find_first_not_of(" \t\r\n") != std::string::npos) {
                count++;
            }
        }
        return count;
    }

    std::vector<std::string> getLastN(size_t n) const {
        std::vector<std::string> result;
        if (n == 0) return result;

        std::ifstream infile(filepath_);
        if (!infile.is_open()) return result;

        std::deque<std::string> ring;
        std::string line;
        while (std::getline(infile, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
                continue;
            }

            if (ring.size() == n) {
                ring.pop_front();
            }
            ring.push_back(line);
        }

        result.assign(ring.begin(), ring.end());
        return result;
    }

    size_t displayLastN(size_t n) const {
        auto entries = getLastN(n);
        if (entries.empty()) {
            std::cout << "\n+--------------------------------------------------------------+\n"
                      << "| [LOGBOOK] Log file is currently empty or does not exist.     |\n"
                      << "+--------------------------------------------------------------+\n\n";
            return 0;
        }

        size_t total = countEntries();
        std::cout << "\n================================================================\n"
                  << "                  LAST " << entries.size() << " LOGBOOK ENTRIES (C++)\n";
        if (total > entries.size()) {
            std::cout << "       (Showing " << entries.size() << " of " << total << " total entries in chronological order)\n";
        } else {
            std::cout << "                (Showing all " << entries.size() << " available entries)\n";
        }
        std::cout << "================================================================\n";

        for (size_t i = 0; i < entries.size(); ++i) {
            std::cout << "[" << std::setw(3) << (i + 1) << "] " << entries[i] << "\n";
        }
        std::cout << "================================================================\n\n";
        return entries.size();
    }

    size_t displayAll() const {
        std::ifstream infile(filepath_);
        if (!infile.is_open()) {
            std::cout << "\n[LOGBOOK] File '" << filepath_ << "' does not exist or is empty.\n\n";
            return 0;
        }

        std::cout << "\n================================================================\n"
                  << "                   ALL LOGBOOK ENTRIES (" << filepath_ << ")\n"
                  << "================================================================\n";

        std::string line;
        size_t count = 0;
        while (std::getline(infile, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            if (!line.empty()) {
                count++;
                std::cout << "[" << std::setw(4) << count << "] " << line << "\n";
            }
        }
        std::cout << "================================================================\n"
                  << " Total entries displayed: " << count << "\n\n";
        return count;
    }

    size_t search(const std::string& keyword) const {
        std::ifstream infile(filepath_);
        if (!infile.is_open()) {
            std::cout << "[LOGBOOK] File '" << filepath_ << "' not found.\n";
            return 0;
        }

        std::string lower_kw = keyword;
        std::transform(lower_kw.begin(), lower_kw.end(), lower_kw.begin(), ::tolower);

        std::cout << "\n================================================================\n"
                  << " SEARCH RESULTS FOR: \"" << keyword << "\"\n"
                  << "================================================================\n";

        std::string line;
        size_t line_num = 0, matches = 0;
        while (std::getline(infile, line)) {
            line_num++;
            std::string lower_line = line;
            std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);

            if (lower_line.find(lower_kw) != std::string::npos) {
                matches++;
                std::cout << "Line " << std::setw(4) << line_num << ": " << line << "\n";
            }
        }
        std::cout << "================================================================\n"
                  << " Found " << matches << " match(es) across " << line_num << " lines.\n\n";
        return matches;
    }

    bool clear() {
        std::ofstream outfile(filepath_, std::ios::trunc);
        return outfile.good();
    }

    void displayStats() const {
        size_t total = countEntries();
        uintmax_t size_bytes = 0;
        if (std::filesystem::exists(filepath_)) {
            size_bytes = std::filesystem::file_size(filepath_);
        }

        std::string first_ts = "N/A", last_ts = "N/A";
        std::ifstream infile(filepath_);
        if (infile.is_open()) {
            std::string line;
            size_t idx = 0;
            while (std::getline(infile, line)) {
                if (line.empty() || line[0] != '[') continue;
                auto close_bracket = line.find(']');
                if (close_bracket != std::string::npos) {
                    std::string ts = line.substr(1, close_bracket - 1);
                    if (idx == 0) first_ts = ts;
                    last_ts = ts;
                    idx++;
                }
            }
        }

        std::cout << "\n================================================================\n"
                  << "                  LOGBOOK STATISTICS (C++)                      \n"
                  << "================================================================\n"
                  << " Log File:         " << filepath_ << "\n"
                  << " Total Entries:    " << total << "\n"
                  << " File Size:        " << size_bytes << " bytes ("
                  << std::fixed << std::setprecision(2) << (size_bytes / 1024.0) << " KB)\n"
                  << " First Entry Date: " << first_ts << "\n"
                  << " Last Entry Date:  " << last_ts << "\n"
                  << "================================================================\n\n";
    }
};

int main(int argc, char* argv[]) {
    LogBook lb("logbook.txt");

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [--add <msg>] [--tag <tag>] [--last <N>] [--all] [--search <kw>] [--stats] [--clear]\n";
                return 0;
            } else if ((arg == "--add" || arg == "-a") && i + 1 < argc) {
                std::string msg = argv[++i];
                std::string tag = "";
                if (i + 2 < argc && (std::string(argv[i + 1]) == "--tag" || std::string(argv[i + 1]) == "-t")) {
                    i++;
                    tag = argv[++i];
                }
                lb.append(msg, tag);
                std::cout << "[LOGBOOK] Entry appended.\n";
            } else if ((arg == "--last" || arg == "-n") && i + 1 < argc) {
                size_t n = std::stoul(argv[++i]);
                lb.displayLastN(n);
            } else if (arg == "--all" || arg == "-l") {
                lb.displayAll();
            } else if ((arg == "--search" || arg == "-s") && i + 1 < argc) {
                lb.search(argv[++i]);
            } else if (arg == "--stats") {
                lb.displayStats();
            } else if (arg == "--clear") {
                lb.clear();
                std::cout << "[LOGBOOK] Logbook cleared.\n";
            }
        }
        return 0;
    }

    /* Interactive Menu */
    std::cout << "================================================================\n"
              << "       LOGBOOK APPLICATION (C++ Modern Implementation)         \n"
              << "================================================================\n";

    while (true) {
        std::cout << "\n--- LOGBOOK MENU (" << lb.getFilePath() << ") ---\n"
                  << "  [1] Append New Timestamped Entry\n"
                  << "  [2] Append Entry with Tag (INFO, WARN, ERROR, etc.)\n"
                  << "  [3] Display Last N Entries\n"
                  << "  [4] Display All Entries\n"
                  << "  [5] Search Entries by Keyword\n"
                  << "  [6] View Logbook Statistics\n"
                  << "  [7] Clear Logbook\n"
                  << "  [0] Exit\n"
                  << "Enter choice (0-7): ";

        std::string input;
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;

        int choice = -1;
        try { choice = std::stoi(input); } catch (...) { continue; }

        if (choice == 0) {
            std::cout << "Exiting. Goodbye!\n";
            break;
        }

        switch (choice) {
            case 1: {
                std::cout << "Enter log message: ";
                std::string msg;
                if (std::getline(std::cin, msg) && !msg.empty()) {
                    lb.append(msg);
                    std::cout << "[✓] Entry appended.\n";
                }
                break;
            }
            case 2: {
                std::cout << "Enter tag: ";
                std::string tag;
                std::getline(std::cin, tag);
                std::cout << "Enter log message: ";
                std::string msg;
                if (std::getline(std::cin, msg) && !msg.empty()) {
                    lb.append(msg, tag);
                    std::cout << "[✓] Tagged entry appended.\n";
                }
                break;
            }
            case 3: {
                std::cout << "Enter number of recent entries (N): ";
                std::string n_str;
                if (std::getline(std::cin, n_str)) {
                    try {
                        size_t n = std::stoul(n_str);
                        lb.displayLastN(n);
                    } catch (...) {
                        std::cout << "[!] Invalid integer.\n";
                    }
                }
                break;
            }
            case 4:
                lb.displayAll();
                break;
            case 5: {
                std::cout << "Enter search keyword: ";
                std::string kw;
                if (std::getline(std::cin, kw) && !kw.empty()) {
                    lb.search(kw);
                }
                break;
            }
            case 6:
                lb.displayStats();
                break;
            case 7: {
                std::cout << "Confirm clear? (y/N): ";
                std::string confirm;
                if (std::getline(std::cin, confirm) && (confirm == "y" || confirm == "Y")) {
                    lb.clear();
                    std::cout << "[✓] Logbook cleared.\n";
                }
                break;
            }
            default:
                std::cout << "[!] Invalid choice.\n";
                break;
        }
    }
    return 0;
}

