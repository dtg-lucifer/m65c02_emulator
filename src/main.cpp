#include <iostream>

int main() {
    std::cout << "Hello 65c02" << std::endl;

    // Run a endless loop while the HALT line is
    // low or the reset line gets a pulse
    // also pulsate the clock and feed it to the bus so that the other
    // components can have that
    return 0;
}
