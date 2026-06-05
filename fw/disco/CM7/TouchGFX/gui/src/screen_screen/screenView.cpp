#include <gui/screen_screen/screenView.hpp>
#include <touchgfx/Color.hpp>

#include "stm32h7xx_hal.h"   /* HAL_GetTick() -- ms wall clock */

namespace
{
/* RGB888 colors per LED, off (dim "ambient") and on (bright "lit"). */
struct LedColors
{
    touchgfx::colortype off;
    touchgfx::colortype on;
};

/* Order MUST match screenView::Led enum. */
const LedColors kLedColors[static_cast<int>(screenView::Led::Count)] = {
    /* HighBeam:  off #000012, on #3070FF (blue)  */
    { touchgfx::Color::getColorFromRGB(0x00, 0x00, 0x12),
      touchgfx::Color::getColorFromRGB(0x30, 0x70, 0xFF) },
    /* Indicator: off #001200, on #40C000 (green) */
    { touchgfx::Color::getColorFromRGB(0x00, 0x12, 0x00),
      touchgfx::Color::getColorFromRGB(0x40, 0xC0, 0x00) },
    /* Overtemp:  off #120000, on #C02000 (red)   */
    { touchgfx::Color::getColorFromRGB(0x12, 0x00, 0x00),
      touchgfx::Color::getColorFromRGB(0xC0, 0x20, 0x00) },
    /* Oil:       off #120000, on #C02000 (red)   */
    { touchgfx::Color::getColorFromRGB(0x12, 0x00, 0x00),
      touchgfx::Color::getColorFromRGB(0xC0, 0x20, 0x00) },
    /* Battery:   off #120000, on #C02000 (red)   */
    { touchgfx::Color::getColorFromRGB(0x12, 0x00, 0x00),
      touchgfx::Color::getColorFromRGB(0xC0, 0x20, 0x00) },
};
} /* anonymous namespace */

screenView::screenView()
    : testRpm(0),  testRpmStep(60),
      testFuel(kMinFuelLitres), testFuelStep(1),
      testTemp(kMinTempC),      testTempStep(1)
{
    for (auto& s : ledState)
    {
        s = false;
    }
}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();
}

void screenView::tearDownScreen()
{
    screenViewBase::tearDownScreen();
}

void screenView::setRPM(uint16_t rpm)
{
    if (rpm > kMaxRpm)
    {
        rpm = kMaxRpm;
    }

    /* Designer pose has the needle at ZAngle = kMaxAngle for redline.
     * Linear scale: angle = rpm / kMaxRpm * kMaxAngle. X/Y angles
     * stay 0 -- the gauge only spins in-plane around the origo. */
    const float zAngle = (static_cast<float>(rpm) / static_cast<float>(kMaxRpm)) * kMaxAngle;

    rpm_needle.setAngles(0.0f, 0.0f, zAngle);
    rpm_needle.invalidate();
}

void screenView::setFuel(uint16_t litres)
{
    if (litres > kMaxFuelLitres)
    {
        litres = kMaxFuelLitres;
    }

    /* Linear interpolation between Designer empty/full poses.
     * Note kFuelAngleFull < kFuelAngleEmpty: needle rotates clockwise
     * (decreasing Z angle) as the tank fills. */
    const float t      = static_cast<float>(litres) / static_cast<float>(kMaxFuelLitres);
    const float zAngle = kFuelAngleEmpty + t * (kFuelAngleFull - kFuelAngleEmpty);

    fuel_needle.setAngles(0.0f, 0.0f, zAngle);
    fuel_needle.invalidate();
}

void screenView::setTemperature(int16_t temperatureC)
{
    if (temperatureC < kMinTempC)
    {
        temperatureC = kMinTempC;
    }
    else if (temperatureC > kMaxTempC)
    {
        temperatureC = kMaxTempC;
    }

    /* Linear interpolation between Designer cold/hot poses. */
    const float t      = static_cast<float>(temperatureC - kMinTempC) /
                         static_cast<float>(kMaxTempC - kMinTempC);
    const float zAngle = kTempAngleCold + t * (kTempAngleHot - kTempAngleCold);

    temperature_needle.setAngles(0.0f, 0.0f, zAngle);
    temperature_needle.invalidate();
}

void screenView::handleTickEvent()
{
    /* Sweep RPM 0 -> kMaxRpm -> 0 -> ... at testRpmStep RPM/tick.
     * At 60 fps, step=60 RPM completes a full 0->6000 sweep in ~1.7 s. */
    testRpm += testRpmStep;
    if (testRpm >= static_cast<int32_t>(kMaxRpm))
    {
        testRpm     = kMaxRpm;
        testRpmStep = -testRpmStep;
    }
    else if (testRpm <= 0)
    {
        testRpm     = 0;
        testRpmStep = -testRpmStep;
    }

    setRPM(static_cast<uint16_t>(testRpm));

    /* Sweep fuel 0 -> 70 L -> 0 at testFuelStep L/tick.
     * At 60 fps, step=1 completes a full sweep in ~1.17 s. */
    testFuel += testFuelStep;
    if (testFuel >= static_cast<int32_t>(kMaxFuelLitres))
    {
        testFuel     = kMaxFuelLitres;
        testFuelStep = -testFuelStep;
    }
    else if (testFuel <= static_cast<int32_t>(kMinFuelLitres))
    {
        testFuel     = kMinFuelLitres;
        testFuelStep = -testFuelStep;
    }

    setFuel(static_cast<uint16_t>(testFuel));

    /* Sweep coolant 40 -> 140 C -> 40 at testTempStep C/tick.
     * At 60 fps, step=1 completes a full sweep in ~1.67 s. */
    testTemp += testTempStep;
    if (testTemp >= static_cast<int32_t>(kMaxTempC))
    {
        testTemp     = kMaxTempC;
        testTempStep = -testTempStep;
    }
    else if (testTemp <= static_cast<int32_t>(kMinTempC))
    {
        testTemp     = kMinTempC;
        testTempStep = -testTempStep;
    }

    setTemperature(static_cast<int16_t>(testTemp));

    /* ---- LED test patterns ----
     * Driven from HAL_GetTick() (ms wall clock) so patterns are
     * independent of the variable handleTickEvent rate. All four
     * patterns share the same time base, so they stay phase-locked. */
    const uint32_t t_ms = HAL_GetTick();

    /* Indicator: 1 Hz blink (500 ms on / 500 ms off) for 5 s, then
     * 5 s idle off. */
    {
        const uint32_t phase = t_ms % kIndicatorPeriod;
        bool on = false;
        if (phase < kIndicatorBlinkSpan)
        {
            const uint32_t cyc = phase % (kIndicatorBlinkOn + kIndicatorBlinkOff);
            on = (cyc < kIndicatorBlinkOn);
        }
        led_set(Led::Indicator, on);
    }

    /* High beam: two short 200 ms flashes back-to-back, then 6 s off.
     * Layout within the 6.8 s period:
     *   [0 .. P)            on   (flash 1)
     *   [P .. 2P)           off
     *   [2P .. 3P)          on   (flash 2)
     *   [3P .. period)      off  (covers final pulse + 6 s idle) */
    {
        const uint32_t phase = t_ms % kHighBeamPeriod;
        const bool on = (phase < kHighBeamPulse) ||
                        (phase >= 2U * kHighBeamPulse &&
                         phase <  3U * kHighBeamPulse);
        led_set(Led::HighBeam, on);
    }

    /* Battery: 50% duty cycle, on for 5 s then off for 5 s. */
    {
        const uint32_t phase = t_ms % kBatteryPeriod;
        led_set(Led::Battery, phase < kBatteryHalfPeriod);
    }

    /* Oil: brief 200 ms flash once every 5 s. */
    {
        const uint32_t phase = t_ms % kOilPeriod;
        led_set(Led::Oil, phase < kOilFlash);
    }
}

void screenView::led_set(Led led, bool on)
{
    const int idx = static_cast<int>(led);
    if (idx < 0 || idx >= static_cast<int>(Led::Count))
    {
        return;
    }

    /* Suppress redundant invalidates: TouchGFX would otherwise dirty
     * the rect every frame the test pattern keeps the LED in the
     * same state, which is wasted DMA2D + LTDC bandwidth. */
    if (ledState[idx] == on)
    {
        return;
    }
    ledState[idx] = on;

    const touchgfx::colortype color =
        on ? kLedColors[idx].on : kLedColors[idx].off;

    /* Pick the matching painter+circle pair. The painter holds the
     * fill color; the Circle holds geometry. We update the color and
     * tell the Circle to re-fetch from its painter, then invalidate. */
    switch (led)
    {
    case Led::HighBeam:
        led_high_beamPainter.setColor(color);
        led_high_beam.setPainter(led_high_beamPainter);
        led_high_beam.invalidate();
        break;
    case Led::Indicator:
        led_indicatorPainter.setColor(color);
        led_indicator.setPainter(led_indicatorPainter);
        led_indicator.invalidate();
        break;
    case Led::Overtemp:
        led_overtempPainter.setColor(color);
        led_overtemp.setPainter(led_overtempPainter);
        led_overtemp.invalidate();
        break;
    case Led::Oil:
        led_oilPainter.setColor(color);
        led_oil.setPainter(led_oilPainter);
        led_oil.invalidate();
        break;
    case Led::Battery:
        led_batteryPainter.setColor(color);
        led_battery.setPainter(led_batteryPainter);
        led_battery.invalidate();
        break;
    case Led::Count:
    default:
        break;
    }
}
