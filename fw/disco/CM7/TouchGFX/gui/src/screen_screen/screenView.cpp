#include <gui/screen_screen/screenView.hpp>
#include <touchgfx/Color.hpp>

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
