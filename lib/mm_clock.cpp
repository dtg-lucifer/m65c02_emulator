#include "mm_clock.h"

void MM_ClockModule::step() {
    this->CLK = !this->CLK;  // Toggle the clock pin
}
