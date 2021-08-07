#pragma once

namespace adc {
    static constexpr uint8_t Resolution = 10;
    static constexpr uint16_t MaxValue = 1 << Resolution;
    static constexpr float ReferenceVoltage = 3.3f;

    inline constexpr uint16_t voltageToValue(float voltage, float dividerRatio) {
        return static_cast<uint16_t>(round(voltage * MaxValue / ReferenceVoltage * dividerRatio));
    }

    inline constexpr float valueToVoltage(uint16_t value, float dividerRatio) {
        return value * ReferenceVoltage / dividerRatio / MaxValue;
    }
}
