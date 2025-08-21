#include "log.h"

int main() {
    log::print("WDC65C02 Computer Simulator");
    log::info("Hello");

    // Run a endless loop while the HALT line is
    // low or the reset line gets a pulse
    // also pulsate the clock and feed it to the bus so that the other
    // components can have that
    //
    // Basically each component is started into different threads
    // so they can run infinitely until and unless the main program is closed
    // or the CPU gets a HALT signal so that every component will pause execution
    // except the clock.
    return 0;
}
