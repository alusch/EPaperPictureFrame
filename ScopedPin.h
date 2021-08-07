#pragma once

// RAII helper to simplify toggling pins high and low.
template<uint8_t Initial, uint8_t Final>
class ScopedPin final {
public:
    ScopedPin(uint8_t pin) : pin(pin) {
        digitalWrite(pin, Initial);
    }

    ~ScopedPin() {
        digitalWrite(pin, Final);
    }

private:
    const uint8_t pin;
};

using ScopedHighPin = ScopedPin<HIGH, LOW>;
using ScopedLowPin = ScopedPin<LOW, HIGH>;
