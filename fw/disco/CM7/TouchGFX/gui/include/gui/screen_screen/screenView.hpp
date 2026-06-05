#ifndef SCREENVIEW_HPP
#define SCREENVIEW_HPP

#include <gui_generated/screen_screen/screenViewBase.hpp>
#include <gui/screen_screen/screenPresenter.hpp>

class screenView : public screenViewBase
{
public:
    screenView();
    virtual ~screenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
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

protected:
    /* Test pattern: sweep RPM 0 -> kMaxRpm -> 0 to exercise the
     * TextureMapper redraw path through the partial-FB pipeline. */
    static constexpr uint16_t kMaxRpm   = 6000;
    static constexpr float    kMaxAngle = 3.65f;

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

    int32_t  testRpm;      /* signed so we can detect under/overshoot */
    int16_t  testRpmStep;  /* +N -> accel, -N -> decel */

    int32_t  testFuel;     /* litres, swept across [kMin..kMax]FuelLitres */
    int16_t  testFuelStep;

    int32_t  testTemp;     /* deg C, swept across [kMin..kMax]TempC      */
    int16_t  testTempStep;
};

#endif // SCREENVIEW_HPP
