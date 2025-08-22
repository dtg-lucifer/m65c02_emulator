#ifndef LOG_H
#define LOG_H

#include <iomanip>
#include <iostream>

#include "colors.h"

namespace logger {

inline void info(std::string msg) {
    std::cout << colors::BOLD << colors::GREEN << std::setw(11) << std::setfill(' ') << "[INFO] " << colors::RESET
              << msg << std::endl;
}

inline void error(std::string msg) {
    std::cerr << colors::BOLD << colors::RED << std::setw(11) << std::setfill(' ') << "[ERROR] " << colors::RESET << msg
              << std::endl;
}

inline void warning(std::string msg) {
    std::cerr << colors::BOLD << colors::YELLOW << std::setw(11) << std::setfill(' ') << "[WARNING] " << colors::RESET
              << msg << std::endl;
}

inline void debug(std::string msg) {
    std::cout << colors::BOLD << colors::BLUE << std::setw(11) << std::setfill(' ') << "[DEBUG] " << colors::RESET
              << msg << std::endl;
}

inline void print(std::string msg) {
    std::cout << colors::BOLD << colors::GREEN << std::setw(11) << std::setfill(' ') << "[WDC65C02] " << colors::RESET
              << msg << std::endl;
}

inline void header(std::string msg) {
    std::cout << colors::BOLD << colors::MAGENTA << std::string(60, '=') << colors::RESET << std::endl;
    std::cout << colors::BOLD << colors::MAGENTA << std::setw(30 + (msg.length() / 2)) << std::setfill(' ') << msg
              << colors::RESET << std::endl;
    std::cout << colors::BOLD << colors::MAGENTA << std::string(60, '=') << colors::RESET << std::endl;
}

inline void subheader(std::string msg) {
    std::cout << colors::BOLD << colors::CYAN << std::string(50, '-') << colors::RESET << std::endl;
    std::cout << colors::BOLD << colors::CYAN << std::setw(25 + (msg.length() / 2)) << std::setfill(' ') << msg
              << colors::RESET << std::endl;
    std::cout << colors::BOLD << colors::CYAN << std::string(50, '-') << colors::RESET << std::endl;
}

inline void divider() {
    std::cout << colors::BOLD << colors::WHITE << std::string(60, '-') << colors::RESET << std::endl;
}

}  // namespace logger

#endif  // LOG_H
