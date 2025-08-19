#ifndef CLOCK_H
#define CLOCK_H

#include "types.h"

class MM_ClockModule {
   private:
    float speed;

   public:
    MM_ClockModule(float s);

    // Pins for connecting to the BUS
    union {
        clock_pin_t PIN;
        struct {
            clock_pin_t GRD : 1;
            clock_pin_t VCC : 1;
            clock_pin_t CLK : 1;
        };
    };

    // Get the current speed
    const float get_speed();

    // Set the current clock speed
    void set_speed(const float speed);

    // Step the clock manually one clock
    void step();

    // Set the clock in A-STABLE mode
    //
    // Note: Continuous clock cycles
    void sasm();
    // Set the clock in Mono-STABLE mode
    //
    // Note: Manual stepping mode
    // you have to call the `step` method
    // to step the clock for the next pulse
    void smsm();
};

#endif  // CLOCK_H
