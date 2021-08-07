#include <ArduinoLowPower.h>
#include <RTCZero.h>
#include <SdFat.h>

#include "Adc.h"
#include "EPaperDisplay.h"
#include "SdEnumerator.h"
#include "SdImageViewer.h"
#include "SPIClassSercom.h"
#include "Util.h"

// Set to true to make it easier to debug (see below for details).
static constexpr bool DebugMode = false;

namespace power {
    namespace pins {
        static constexpr uint8_t BatteryVoltage = A7;
        static constexpr uint8_t UsbVoltage = A5;
        static constexpr uint8_t Led = 13;
    }

    static constexpr float BatteryVoltageThreshold = adc::voltageToValue(3.5f, 0.5f);
    static constexpr float UsbVoltageThreshold = adc::voltageToValue(4.0f, 0.5f);
}

namespace display {
    static constexpr uint16_t Width = 600;
    static constexpr uint16_t Height = 448;
}

namespace file {
    static constexpr uint64_t ExpectedSize = display::Width * display::Height / 2;

    // Image loading will pick up at the next file after this one. Generally set to the empty string to start
    // at the beginning, but can be changed if you need to re-flash and want to keep going where you left off.
    static constexpr const char* PreviousFileName = "";
}

namespace epaper {
    namespace pins {
        static constexpr uint8_t Miso = 10;
        static constexpr uint8_t Clk = 12;
        static constexpr uint8_t Mosi = 11;
        static constexpr uint8_t Cs = 15;
        static constexpr uint8_t Dc = 16;
        static constexpr uint8_t Reset = 17;
        static constexpr uint8_t Busy = 18;

        static constexpr PinAssignment MisoAssignment = PinAssignment(Miso, PIO_SERCOM);
        static constexpr PinAssignment ClkAssignment = PinAssignment(Clk, PIO_SERCOM);
        static constexpr PinAssignment MosiAssignment = PinAssignment(Mosi, PIO_SERCOM);

        static constexpr SercomSpiTXPad TxPad = SPI_PAD_0_SCK_3;
        static constexpr SercomRXPad RxPad = SERCOM_RX_PAD_2;
    }

    static constexpr uint32_t SpiSpeed = 2e6;
}

namespace sdcard {
    namespace pins {
        static constexpr uint8_t Cs = 4;
        static constexpr uint8_t Detect = 7;
        static constexpr uint8_t Led = 8;
    }

    static constexpr uint32_t SpiSpeed = 12e6;

    static constexpr uint32_t DelayAfterInserted = 5_s;
}

namespace rtc {
    namespace initial {
        // Lazy hack to avoid time-setting UI:
        // When reset, initialize the RTC to this time. The user is then responsible for
        // powering up or hitting the reset button at the correct actual time of day.
        static constexpr uint8_t Hour = 12;
        static constexpr uint8_t Minute = 0;
        static constexpr uint8_t Second = 0;
    }

    namespace alarm {
        // Wake up to advance images at 3 AM or so (not going to bother accounting for Daylight Savings).
        static constexpr uint8_t Hour = 3;
        static constexpr uint8_t Minute = 0;
        static constexpr uint8_t Second = 0;

        // When debugging, match on seconds only (i.e. wake up every minute) for ease of testing.
        // In normal operation, match on hour, minute, and second (i.e. wake up every day)
        // to give some time to enjoy the images.
        static constexpr RTCZero::Alarm_Match Match = DebugMode ? RTCZero::MATCH_SS : RTCZero::MATCH_HHMMSS;
    }
}

// Set up a new SPI bus on SERCOM 1 for the display.
static SPIClassSercom ePaperSPI(
    &sercom1, epaper::pins::MisoAssignment, epaper::pins::ClkAssignment, epaper::pins::MosiAssignment,
    epaper::pins::TxPad, epaper::pins::RxPad);

static const EPaperDisplay ePaper(
    ePaperSPI, epaper::SpiSpeed,
    epaper::pins::Cs, epaper::pins::Dc, epaper::pins::Reset, epaper::pins::Busy,
    display::Width, display::Height);

static SdEnumerator sd(file::PreviousFileName, sdcard::SpiSpeed, sdcard::pins::Cs, file::ExpectedSize);
static SdImageViewer viewer(ePaper, sd, sdcard::pins::Led);

static RTCZero realTimeClock;

enum class WakeReason : uint8_t {
    None = 0,
    Timer,
    UsbPluggedIn,
    UsbUnplugged,
    BatteryBelowThreshold,
    SdCardInserted,
};

// Default to Timer so we show the first image upon initial power-on.
static volatile WakeReason wakeReason = WakeReason::Timer;

void setup() {
    pinMode(power::pins::BatteryVoltage, INPUT);
    pinMode(power::pins::UsbVoltage, INPUT);
    pinMode(power::pins::Led, OUTPUT);

    pinMode(sdcard::pins::Detect, INPUT_PULLUP);

    // Turn off power LED to conserve a little power.
    digitalWrite(power::pins::Led, LOW);

    realTimeClock.begin(/* resetTime */ false);
    realTimeClock.setTime(rtc::initial::Hour, rtc::initial::Minute, rtc::initial::Second);
    realTimeClock.setAlarmTime(rtc::alarm::Hour, rtc::alarm::Minute, rtc::alarm::Second);
    realTimeClock.attachInterrupt([] { wakeReason = WakeReason::Timer; });

    sd.setup();
    ePaperSPI.begin();
    ePaper.setup();
    viewer.setup();
}

void loop() {
    const bool batteryGood = analogRead(power::pins::BatteryVoltage) > power::BatteryVoltageThreshold;
    const bool onUsbPower = analogRead(power::pins::UsbVoltage) > power::UsbVoltageThreshold;

    if (onUsbPower || batteryGood) {
        switch (wakeReason) {
            case WakeReason::Timer:
                // If this is the regularly-scheduled wakeup, then load and show the next image.
                viewer.showNextImage();
                break;

            case WakeReason::UsbPluggedIn:
                // If we were showing the low battery warning and power was plugged in, resume showing the current image.
                viewer.showCurrentImage();
                break;

            case WakeReason::SdCardInserted:
                // The SD card detect switch is really sensitive, so wait a few seconds to give us a chance to reach steady state,
                // then see if we actually still think there's a card. If so, reload the current image.
                delay(sdcard::DelayAfterInserted);
                if (digitalRead(sdcard::pins::Detect) == HIGH) {
                    viewer.showCurrentImage();
                }
                break;

            default:
                // Other cases don't need to update the displayed image.
                break;
        }

        wakeReason = WakeReason::None;

        // Wake if an SD card is inserted so we can reload the current image from it. This lets us recover cases where other triggers occurred
        // while a card wasn't present, and also gives an easy way to edit the current image and reload it when the card is reinserted.
        // Note: This must be set up *before* the ADC interrupt, otherwise attachInterruptWakeup would disable the clock for the ADC and cause hangs.
        LowPower.attachInterruptWakeup(sdcard::pins::Detect, [] { wakeReason = WakeReason::SdCardInserted; }, RISING);

        if (onUsbPower) {
            // If currently on USB power, wake when the USB is unplugged so we can go back to the low battery warning if necessary.
            LowPower.attachAdcInterrupt(power::pins::UsbVoltage, [] { wakeReason = WakeReason::UsbUnplugged; }, ADC_INT_BELOW_MAX, 0, power::UsbVoltageThreshold);
        } else {
            // And if currently on battery power, wake if the voltage drops below the threshold so we can show the warning.
            LowPower.attachAdcInterrupt(power::pins::BatteryVoltage, [] { wakeReason = WakeReason::BatteryBelowThreshold; }, ADC_INT_BELOW_MAX, 0, power::BatteryVoltageThreshold);
        }

        realTimeClock.enableAlarm(rtc::alarm::Match);

        // Wait in a low power state for our next scheduled picture flip or one of the other events.
        LowPower.deepSleep();

        realTimeClock.disableAlarm();
        LowPower.detachAdcInterrupt();
        detachInterrupt(sdcard::pins::Detect);
    } else {
        wakeReason = WakeReason::None;

        // Running on battery and the voltage is getting low, so show a warning.
        viewer.showLowBatteryImage();

        // Wait indefinitely in the low power state for USB to be plugged in.
        LowPower.attachAdcInterrupt(power::pins::UsbVoltage, [] { wakeReason = WakeReason::UsbPluggedIn; }, ADC_INT_ABOVE_MIN, power::UsbVoltageThreshold, 0);
        LowPower.deepSleep();
        LowPower.detachAdcInterrupt();
    }
}
