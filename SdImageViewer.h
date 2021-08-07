#pragma once

#include "EPaperDisplay.h"
#include "SdEnumerator.h"

class SdImageViewer final {
public:
    SdImageViewer(const EPaperDisplay& ePaper, SdEnumerator& sd, uint8_t ledPin) :
        ePaper(ePaper), sd(sd), ledPin(ledPin)
    {}

    void setup() const;

    void showNextImage();
    void showCurrentImage();

    void showLowBatteryImage();

private:
    void displayImage(FsFile& file, Color fallbackColor) const;
    void showSystemImage(const char* fileName, Color fallbackColor);

private:
    const EPaperDisplay& ePaper;
    SdEnumerator& sd;
    const uint8_t ledPin;
};
