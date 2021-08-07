#pragma once

#include <SdFat.h>

// Enumerates files in the root of the SD card, returning those suitable to show on the EPaper display.
// In order to return files in order without sorting, scans all files in the root directory on every pass.
class SdEnumerator final {
public:
    SdEnumerator(const char* previousFileName, uint32_t spiSpeed, uint8_t csPin, uint64_t expectedFileSize);

    // One-time setup.
    void setup() const;

    // Initialize the card for use. Call each time after resuming from sleep to ensure information is up to date.
    bool begin();

    // Retrieve the next image on the card, looping around at the end.
    // If no suitable files are present, the returned image will not be open.
    FsFile getNextImage();

    // Retrieve the current image in the enumeration, falling back to the next
    // image or the first image if it is no longer available. If no suitable
    // files are present, the returned image will not be open.
    FsFile getCurrentImage();

    // Retrieves a specific system image, separate from the normal rotation, e.g. for low battery.
    FsFile getSystemImage(const char* fileName);

private:
    bool isSuitable(FsFile& file) const;

private:
    static constexpr uint32_t MaxFileNameLength = 256;

    const uint32_t spiSpeed;
    const uint8_t csPin;
    const uint64_t expectedFileSize;
    SdFs sd;
    char currentFileName[MaxFileNameLength];
};
