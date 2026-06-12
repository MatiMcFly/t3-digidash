#include <gui/screen_screen/screenView.hpp>
#include <touchgfx/Color.hpp>

/* For JOY_SEL_Pin / JOY_SEL_GPIO_Port and HAL_GPIO_ReadPin(). main.h
 * pulls in stm32h7xx_hal.h transitively. */
extern "C"
{
#include "main.h"
}

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
    joySelPrev = false;
}

void screenView::setupScreen()
{
    screenViewBase::setupScreen();

    /* Re-arm the joystick edge detector on screen entry. Not pressed = low */
    joySelPrev = (HAL_GPIO_ReadPin(JOY_SEL_GPIO_Port, JOY_SEL_Pin) ==
                  GPIO_PIN_RESET);

    /* Paint LEDs to a known safe-default state at screen entry. Until
     * the first matching telemetry sample arrives, the dashboard
     * shows the oil and battery warning lamps LIT ("unknown == bad")
     * and the rest dark. We force the cache to the opposite of the
     * desired state so led_set() actually invalidates this frame --
     * the constructor leaves ledState[] all-false. */
    static constexpr struct { Led led; bool on; } kDefaults[] = {
        { Led::HighBeam,  false },
        { Led::Indicator, false },
        { Led::Overtemp,  false },
        { Led::Oil,       true  },
        { Led::Battery,   true  },
    };
    for (const auto& d : kDefaults)
    {
        ledState[static_cast<int>(d.led)] = !d.on;
        led_set(d.led, d.on);
    }
}

void screenView::tearDownScreen()
{
    screenViewBase::tearDownScreen();
}

void screenView::handleTickEvent()
{
    screenViewBase::handleTickEvent();

    /* edge detection for JOY_SEL (PK2). Pressed = LOW, not pressed = HIGH. */
    const bool joySelNow =
        (HAL_GPIO_ReadPin(JOY_SEL_GPIO_Port, JOY_SEL_Pin) ==
         GPIO_PIN_RESET);

    if (joySelNow && !joySelPrev)
    {
        /* if the joystick was just pressed, toggle which middle-display container is shown */
        const bool show2 = !cont_middle_disp_2.isVisible();
        cont_middle_disp_1.setVisible(!show2);
        cont_middle_disp_2.setVisible( show2);
        cont_middle_disp_1.invalidate();
        cont_middle_disp_2.invalidate();
    }

    joySelPrev = joySelNow;
}

void screenView::setRPM(uint16_t rpm)
{
    if (rpm > kMaxRpm)
    {
        rpm = kMaxRpm;
    }

    /* Piecewise-linear mapping that matches the reference dial:
     *   0    ..  kKinkRpm  ->  0          .. kKinkAngle   (compressed
     *                                                      idle arc)
     *   kKinkRpm .. kMaxRpm ->  kKinkAngle .. kMaxAngle    (main scale,
     *                                                      1k..6k RPM)
     * X/Y angles stay 0 -- the gauge only spins in-plane around the
     * origo. */
    float zAngle;
    if (rpm <= kKinkRpm)
    {
        const float t = static_cast<float>(rpm) /
                        static_cast<float>(kKinkRpm);
        zAngle = t * kKinkAngle;
    }
    else
    {
        const float t = static_cast<float>(rpm - kKinkRpm) /
                        static_cast<float>(kMaxRpm - kKinkRpm);
        zAngle = kKinkAngle + t * (kMaxAngle - kKinkAngle);
    }

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

namespace
{
/* Formats a fixed-point value in tenths into a Unicode buffer as
 * "<int>.<frac>" with one decimal place. The buffer must already
 * have been bound to its TextArea via setWildcard1(); the caller is
 * responsible for invalidating the widget after this returns. */
void formatTenths(touchgfx::Unicode::UnicodeChar* buf,
                  uint16_t                        bufSize,
                  int16_t                         tenths)
{
    /* Split into whole + fractional. Use unsigned arithmetic on the
     * absolute value so the modulo always gives a non-negative
     * single decimal digit even for negative inputs (in C, sign of
     * % on negatives is implementation-defined for signed). */
    const bool     neg   = (tenths < 0);
    const uint16_t mag   = static_cast<uint16_t>(neg ? -tenths : tenths);
    const uint16_t whole = mag / 10u;
    const uint16_t frac  = mag % 10u;

    if (neg)
    {
        touchgfx::Unicode::snprintf(buf, bufSize, "-%u.%u", whole, frac);
    }
    else
    {
        touchgfx::Unicode::snprintf(buf, bufSize, "%u.%u", whole, frac);
    }
}
} /* anonymous namespace */

void screenView::setVoltageDeciV(int16_t deciV)
{
    formatTenths(voltageBuffer, VOLTAGE_SIZE, deciV);
    voltage.invalidate();
}

void screenView::setFuelDeciL(int16_t deciL)
{
    formatTenths(fuel_lvlBuffer, FUEL_LVL_SIZE, deciL);
    fuel_lvl.invalidate();
}

void screenView::setTemperatureDeciC(int16_t deciC)
{
    formatTenths(temperatureBuffer, TEMPERATURE_SIZE, deciC);
    temperature.invalidate();
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
