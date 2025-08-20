#include "mm_clock.h"

#include <chrono>
#include <thread>

MM_ClockModule::MM_ClockModule(float s) : speed(s) {
    // Initialize pins
    PIN = 0;
    VCC = 1;  // Power on
    GRD = 0;  // Ground
    CLK = 0;  // Start with clock low
}

const float MM_ClockModule::get_speed() {
    return speed;
}

void MM_ClockModule::set_speed(const float new_speed) {
    speed = new_speed;
}

void MM_ClockModule::step() {
    // Toggle clock signal
    CLK = !CLK;

    // In a hardware implementation, we would wait here based on speed
    // For simulation, we can add a small delay
    // A real clock at 1 MHz would have a period of 1 microsecond
    // Since we're using a stepped simulation, we'll keep this minimal
}

void MM_ClockModule::sasm() {
    // A-Stable mode: continuous clock cycles
    // In a real implementation, this would start an oscillator
    // For our simulation, we'll just note that we're in this mode
    // and rely on the main loop to call step()
}

void MM_ClockModule::smsm() {
    // Mono-Stable mode: manual stepping
    // Reset the clock to 0 and require manual stepping
    CLK = 0;
    // In a real implementation, this would disable the oscillator
}
