#ifndef SCREENVIEW_HPP
#define SCREENVIEW_HPP

#include <gui_generated/screen_screen/screenViewBase.hpp>
#include <gui/screen_screen/screenPresenter.hpp>

class screenView : public screenViewBase
{
public:
    /* Identifies one of the 5 indicator LEDs on the dashboard. */
    enum class Led : uint8_t
    {
        HighBeam,
        Indicator,
        Overtemp,
        Oil,
        Battery,
        Count
    };

    screenView();
    virtual ~screenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* Polled every frame (~16 ms). Edge-detects the DISCO joystick
     * SEL push (PK2, active high on the board's external pull-down)
     * and on each rising edge alternates which middle-display
     * container (cont_middle_disp_1 / cont_middle_disp_2) is
     * visible. */
    virtual void handleTickEvent();

    /* Maps an RPM value (0 .. kMaxRpm) to the rpm_needle Z angle.
     * 0 RPM      -> 0.0 rad
     * kMaxRpm    -> kMaxAngle rad (matches Designer redline pose) */
    void setRPM(uint16_t rpm);

    /* Maps a fuel level in litres (kMinFuelLitres .. kMaxFuelLitres) to
     * the fuel_needle Z angle.
     * 0 L  (empty) -> kFuelAngleEmpty rad
     * 70 L (full)  -> kFuelAngleFull  rad */
    void setFuel(uint16_t litres);

    /* Maps a coolant temperature in deg C (kMinTempC .. kMaxTempC) to
     * the temperature_needle Z angle.
     * 40 C  (cold) -> kTempAngleCold rad
     * 140 C (hot)  -> kTempAngleHot  rad */
    void setTemperature(int16_t temperatureC);

    /* Updates the alphanumeric readouts in cont_middle_disp_2.
     * Each takes a fixed-point value in TENTHS of the displayed unit (deci) */
    void setVoltageDeciV(int16_t deciV);
    void setFuelDeciL(int16_t deciL);
    void setTemperatureDeciC(int16_t deciC);

    /* Sets one of the indicator LEDs on/off. The LED's painter is
     * recolored between its "lit" and "dark" tones (see screenView.cpp)
     * and then invalidated so the change is rendered next frame. */
    void led_set(Led led, bool on);

protected:
    /* RPM gauge: piecewise-linear mapping to match the reference dial.
     * The 0 .. 1000 RPM segment is compressed into a small arc on the
     * left of the dial; 1000 .. 6000 RPM spans the rest. */
    static constexpr uint16_t kMaxRpm    = 6000;
    static constexpr uint16_t kKinkRpm   = 1000;
    static constexpr float    kMaxAngle  = 3.65f;
    static constexpr float    kKinkAngle = 0.49f;

    /* Fuel gauge: empty pose at high Z angle, sweeps DOWN as tank fills. */
    static constexpr uint16_t kMinFuelLitres  = 0;
    static constexpr uint16_t kMaxFuelLitres  = 70;
    static constexpr float    kFuelAngleEmpty = 6.2f;
    static constexpr float    kFuelAngleFull  = 5.4f;

    /* Coolant temperature gauge: cold pose at low Z angle, sweeps UP. */
    static constexpr int16_t  kMinTempC      = 40;
    static constexpr int16_t  kMaxTempC      = 140;
    static constexpr float    kTempAngleCold = 0.55f;
    static constexpr float    kTempAngleHot  = 1.40f;

    /* Cached LED on/off state. led_set() short-circuits when the new
     * state matches the cache, so static signals (e.g. high beam off
     * for minutes at a time) don't dirty their rect every frame. */
    bool ledState[static_cast<int>(Led::Count)];

    /* Previous sampled level of JOY_SEL (PK2). Used by
     * handleTickEvent() for rising-edge detection so that holding the
     * joystick centered only counts as one toggle. */
    bool joySelPrev;
};

#endif // SCREENVIEW_HPP
