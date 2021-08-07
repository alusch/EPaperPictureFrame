#include "SdImageViewer.h"

#include "ScopedPin.h"
#include "Util.h"

static constexpr uint32_t ReadBufferSize = 4200;

namespace errorColor {
    static constexpr Color NoImage = Color::Blue;
    static constexpr Color CardFailure = Color::Orange;
    static constexpr Color LowBattery = Color::Red;
}

void SdImageViewer::setup() const {
    pinMode(ledPin, OUTPUT);
}

void SdImageViewer::displayImage(FsFile& file, Color fallbackColor) const {
    ePaper.init();
    ePaper.clear(Color::Clean);

    if (file.isOpen()) {
        ePaper.beginImage();

        uint8_t buf[ReadBufferSize];
        int bytesRead;
        while ((bytesRead = file.read(buf, array_size(buf))) > 0) {
            ePaper.sendImageData(buf, bytesRead);
        }

        file.close();
        ePaper.endImage();
    } else {
        ePaper.clear(fallbackColor);
    }

    ePaper.sleep();
}

void SdImageViewer::showNextImage() {
    FsFile file;
    auto ledOn = ScopedHighPin(ledPin);
    if (sd.begin()) {
        file = sd.getNextImage();
        displayImage(file, errorColor::NoImage);
    } else {
        displayImage(file, errorColor::CardFailure);
    }
}

void SdImageViewer::showCurrentImage() {
    FsFile file;
    auto ledOn = ScopedHighPin(ledPin);
    if (sd.begin()) {
        file = sd.getCurrentImage();
        displayImage(file, errorColor::NoImage);
    } else {
        displayImage(file, errorColor::CardFailure);
    }
}

void SdImageViewer::showSystemImage(const char* fileName, Color fallbackColor) {
    FsFile file;
    auto ledOn = ScopedHighPin(ledPin);
    if (sd.begin()) {
        file = sd.getSystemImage(fileName);
    }
    displayImage(file, fallbackColor);
}

void SdImageViewer::showLowBatteryImage() {
    showSystemImage("LowBattery.bin", errorColor::LowBattery);
}
