#include "mm_clock.h"

#include <atomic>
#include <chrono>
#include <thread>

#include "log.h"

// Class member to hold the thread
std::thread clock_thread;
std::atomic<bool> running{false};

const float MM_ClockModule::get_speed() {
    return this->speed;
}

void MM_ClockModule::set_speed(const float new_speed) {
    this->speed = new_speed;
}

void MM_ClockModule::step() {
    this->CLK = !this->CLK;  // Toggle the clock pin
}

void MM_ClockModule::start() {
    // Don't start if already running
    if (running.load()) {
        // Silently return if already running
        return;
    }

    // Set running flag to true
    running.store(true);

    // Start the clock in a separate thread
    clock_thread = std::thread([this]() {
        logger::info("Clock started with speed: " + std::to_string(this->speed) + " Hz");

        // Calculate sleep time in microseconds based on clock speed
        auto sleep_time_us = static_cast<int>(1000000 / (2 * this->speed));

        // Keep track of last mode to detect changes
        ClockMode last_mode = this->mode;

        while (running.load()) {
            // Check if in A_STABLE mode (continuous) - in MONO_STABLE, we don't auto-tick
            if (this->mode == ClockMode::A_STABLE) {
                this->step();  // Toggle the clock

                // Sleep for the calculated time to achieve the desired frequency
                std::this_thread::sleep_for(std::chrono::microseconds(sleep_time_us));
            } else {
                // In MONO_STABLE mode, we just wait for external step() calls
                // Only log when mode changes to reduce noise
                if (last_mode != this->mode) {
                    logger::info("Clock in MONO_STABLE mode - waiting for manual steps");
                    last_mode = this->mode;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        // Only log at end of thread
        logger::info("Clock thread stopped");
    });

    // Detach the thread so it can run independently
    clock_thread.detach();
}

void MM_ClockModule::sasm() {
    this->mode = ClockMode::A_STABLE;
    // Set mode silently
}

void MM_ClockModule::smsm() {
    this->mode = ClockMode::MONO_STABLE;
    // Set mode silently
}
