#include "SdEnumerator.h"
#include "Util.h"

SdEnumerator::SdEnumerator(const char* previousFileName, uint32_t spiSpeed, uint8_t csPin, uint64_t expectedFileSize) :
    spiSpeed(spiSpeed),
    csPin(csPin),
    expectedFileSize(expectedFileSize)
{
    strlcpy(currentFileName, previousFileName, array_size(currentFileName));
}

void SdEnumerator::setup() const {
    pinMode(csPin, OUTPUT);
}

bool SdEnumerator::begin() {
    return sd.begin(csPin, spiSpeed);
}

FsFile SdEnumerator::getNextImage() {
    FsFile root = sd.open("/");
    FsFile file;

    char firstFileName[MaxFileNameLength];
    uint32_t firstIndex = UINT_MAX;

    char nextFileName[MaxFileNameLength];
    uint32_t nextIndex = UINT_MAX;

    while (file.openNext(&root)) {
        if (!isSuitable(file)) {
            file.close();
            continue;
        }

        char candidateFileName[MaxFileNameLength];
        file.getName(candidateFileName, array_size(candidateFileName));

        if (firstIndex == UINT_MAX || strcmp(candidateFileName, firstFileName) < 0) {
            strlcpy(firstFileName, candidateFileName, array_size(firstFileName));
            firstIndex = file.dirIndex();
        }

        if (strcmp(candidateFileName, currentFileName) > 0 && (nextIndex == UINT_MAX || strcmp(candidateFileName, nextFileName) < 0)) {
            strlcpy(nextFileName, candidateFileName, array_size(nextFileName));
            nextIndex = file.dirIndex();
        }

        file.close();
    }

    if (nextIndex == UINT_MAX) {
        nextIndex = firstIndex;
    }

    if (nextIndex != UINT_MAX) {
        file.open(&root, nextIndex, O_RDONLY);
        file.getName(currentFileName, array_size(currentFileName));
    } else {
        currentFileName[0] = '\0';
    }

    root.close();
    return file;
}

FsFile SdEnumerator::getCurrentImage() {
    FsFile root = sd.open("/");
    FsFile file;
    file.open(&root, currentFileName);
    root.close();
    if (!file.isOpen()) {
        file = getNextImage();
    }
    return file;
}

FsFile SdEnumerator::getSystemImage(const char* fileName) {
    FsFile system = sd.open("system");
    FsFile file;
    file.open(&system, fileName);
    system.close();
    return file;
}

bool SdEnumerator::isSuitable(FsFile& file) const {
    return file.isOpen() && !file.isDir() && !file.isHidden() && file.fileSize() == expectedFileSize;
}
