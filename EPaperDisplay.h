#pragma once

#include <SPI.h>

enum class Color : uint8_t {
    Black = 0,
    White = 1,
    Green = 2,
    Blue = 3,
    Red = 4,
    Yellow = 5,
    Orange = 6,
    Clean = 7,
};

// 5.65-inch 7-color EPaper display.
// Assumes it has exclusive use of the provided SPI bus between init and sleep.
class EPaperDisplay final {
public:
    EPaperDisplay(SPIClass& spi, uint32_t spiSpeed, uint8_t csPin, uint8_t dcPin, uint8_t resetPin, uint8_t busyPin, uint16_t width, uint16_t height);

    // One-time setup.
    void setup() const;

    // Reset the display and prepare it for use.
    // Should be called initially and to wake from sleep.
    void init() const;

    // Clear the display to a solid color.
    // Use Color::Clean before sending an image to avoid ghosting.
    void clear(Color color) const;

    // Prepare to send image data.
    void beginImage() const;

    // Send two pixels of image data.
    void sendPixelPair(Color color1, Color color2) const;

    // Send a buffer of image data, two pixels packed to a byte.
    void sendImageData(uint8_t* data, size_t count) const;

    // Refresh the display and show the transmitted image.
    void endImage() const;

    // Put the display into a low-power state.
    // Use init() to wake back up.
    void sleep() const;

private:
    void reset() const;

    void sendCommand(uint8_t command) const;
    void sendData(uint8_t data) const;

    void waitForBusyHigh() const;
    void waitForBusyLow() const;

    void sendResolution() const;

private:
    SPIClass& spi;
    const uint32_t spiSpeed;
    const uint8_t csPin;
    const uint8_t dcPin;
    const uint8_t resetPin;
    const uint8_t busyPin;
    const uint16_t width;
    const uint16_t height;
};
