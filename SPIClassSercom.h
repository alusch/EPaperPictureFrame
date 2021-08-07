#pragma once

#include <wiring_private.h>

struct PinAssignment {
    constexpr PinAssignment(uint32_t pin, EPioType type) :
        pin(pin), type(type) {}

    const uint32_t pin;
    const EPioType type;
};

// Extension of the normal SPI class that correctly sets the pin perhipheral type, necessary when reconfiguring SERCOMs.
class SPIClassSercom final : public SPIClass {
public:
    SPIClassSercom(SERCOM* sercom, PinAssignment miso, PinAssignment sck, PinAssignment mosi, SercomSpiTXPad txPad, SercomRXPad rxPad) :
        SPIClass(sercom, miso.pin, sck.pin, mosi.pin, txPad, rxPad),
        miso(miso),
        sck(sck),
        mosi(mosi)
    {}

    void begin() {
        SPIClass::begin();

        pinPeripheral(miso.pin, miso.type);
        pinPeripheral(sck.pin, sck.type);
        pinPeripheral(mosi.pin, mosi.type);
    }

private:
    const PinAssignment miso;
    const PinAssignment sck;
    const PinAssignment mosi;
};
