#ifndef LOG_H
#define LOG_H

#include <iomanip>
#include <iostream>

#include "colors.h"

namespace log {

void info(std::string msg) {
    std::cout << colors::BOLD << colors::GREEN << std::setw(11) << std::setfill(' ') << "[INFO] " << colors::RESET
              << msg << std::endl;
}

void error(std::string msg) {
    std::cerr << colors::BOLD << colors::RED << std::setw(11) << std::setfill(' ') << "[ERROR] " << colors::RESET << msg
              << std::endl;
}

void print(std::string msg) {
    std::cout << colors::BOLD << colors::GREEN << std::setw(11) << std::setfill(' ') << "[WDC65C02] " << colors::RESET
              << msg << std::endl;
}

}  // namespace log

#endif  // LOG_H
