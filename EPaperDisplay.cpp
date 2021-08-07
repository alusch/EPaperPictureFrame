/*****************************************************************************
* | File      	:   EPD_5in65f.c
* | Author      :   Waveshare team
* | Function    :   5.65inch e-paper
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-07-08
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

// Adapted by Adam Lusch
// For reference see:
// https://www.waveshare.com/w/upload/7/7a/5.65inch_e-Paper_%28F%29_Sepecification.pdf
// https://www.waveshare.com/w/upload/b/b5/5.65inch_e-Paper_%EF%BC%88F%EF%BC%89_Application_Note_Reference_Design.pdf
// https://github.com/waveshare/e-Paper/tree/master/Arduino/epd5in65f/epd5in65f.cpp

#include "EPaperDisplay.h"
#include "Util.h"

EPaperDisplay::EPaperDisplay(SPIClass& spi, uint32_t spiSpeed, uint8_t csPin, uint8_t dcPin, uint8_t resetPin, uint8_t busyPin, uint16_t width, uint16_t height) :
    spi(spi),
    spiSpeed(spiSpeed),
    csPin(csPin),
    dcPin(dcPin),
    resetPin(resetPin),
    busyPin(busyPin),
    width(width),
    height(height)
{}

void EPaperDisplay::setup() const {
    pinMode(csPin, OUTPUT);
    pinMode(dcPin, OUTPUT);
    pinMode(resetPin, OUTPUT);
    pinMode(busyPin, INPUT);
}

void EPaperDisplay::init() const {
    // Assuming exclusive access to the SPI bus while in use
    spi.beginTransaction(SPISettings(spiSpeed, MSBFIRST, SPI_MODE0));
    digitalWrite(csPin, LOW);

    reset();
    waitForBusyHigh();

    sendCommand(0x00);  // Panel setting Register
    sendData(0xEF);     // Scan up, shift right, DC-DC converter on, don't reset
    sendData(0x08);     // Unknown

    sendCommand(0x01);  // Power setting Register
    sendData(0x37);     // Unknown, use internal DC/DC converters
    sendData(0x00);     // Unknown
    sendData(0x23);     // Unknown
    sendData(0x23);     // Unknown

    sendCommand(0x03);  // Power off sequence setting Register
    sendData(0x00);     // 1 frame

    sendCommand(0x06);  // Booster Soft Start
    sendData(0xC7);     // Unknown
    sendData(0xC7);     // Unknown
    sendData(0x1D);     // Unknown

    sendCommand(0x30);  // PLL Control
    sendData(0x3C);     // 50 Hz

    sendCommand(0x41);  // Temperature Sensor Enable
    sendData(0x00);     // Enable internal sensor, 0 offset

    sendCommand(0x50);  // VCOM and Data interval setting
    sendData(0x37);     // Default color LUT, white border, VCOM and data interval 10

    sendCommand(0x60);  // Unknown
    sendData(0x22);     // Unknown

    sendResolution();

    sendCommand(0xE3);  // Unknown
    sendData(0xAA);     // Unknown

    delay(100);

    sendCommand(0x50);  // VCOM and Data interval setting
    sendData(0x37);     // Default color LUT, white border, VCOM and data interval 10
}

void EPaperDisplay::clear(Color color) const {
    beginImage();

    for (int i = 0; i < width / 2 * height; i++) {
        sendPixelPair(color, color);
    }

    endImage();
}

void EPaperDisplay::beginImage() const {
    sendResolution();
    sendCommand(0x10);  // Data Start Transmission
}

void EPaperDisplay::sendPixelPair(Color color1, Color color2) const {
    sendData(((uint8_t)color1 << 4) | (uint8_t)color2);
}

void EPaperDisplay::sendImageData(uint8_t* data, size_t count) const {
    for (size_t i = 0; i < count; i++) {
        sendData(data[i]);
    }
}

void EPaperDisplay::endImage() const {
    sendCommand(0x04);  // Power ON Command
    waitForBusyHigh();

    sendCommand(0x12);  // Display Refresh
    waitForBusyHigh();

    sendCommand(0x02);  // Power OFF Command
    waitForBusyLow();

    delay(500);
}

void EPaperDisplay::sleep() const {
    delay(100);

    sendCommand(0x07);  // Deep sleep
    sendData(0xA5);     // Check code to confirm sleep

    delay(100);

    digitalWrite(resetPin, LOW);

    spi.endTransaction();
    digitalWrite(csPin, HIGH);
}

void EPaperDisplay::reset() const {
    digitalWrite(resetPin, LOW);
    delay(1);
    digitalWrite(resetPin, HIGH);
    delay(200);
}

void EPaperDisplay::sendCommand(uint8_t command) const {
    digitalWrite(dcPin, LOW);
    spi.transfer(command);
}

void EPaperDisplay::sendData(uint8_t data) const {
    digitalWrite(dcPin, HIGH);
    spi.transfer(data);
}

void EPaperDisplay::waitForBusyHigh() const {
    while (digitalRead(busyPin) == LOW) {}
}

void EPaperDisplay::waitForBusyLow() const {
    while (digitalRead(busyPin) == HIGH) {}
}

void EPaperDisplay::sendResolution() const {
    sendCommand(0x61);  // Resolution Setting
    sendData(0x02);     // Width high byte
    sendData(0x58);     // Width low byte
    sendData(0x01);     // Height high byte
    sendData(0xC0);     // Height low byte
}
