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

protected:
    /* Test pattern: sweep RPM 0 -> kMaxRpm -> 0 to exercise the
     * TextureMapper redraw path through the partial-FB pipeline. */
    static constexpr uint16_t kMaxRpm   = 6000;
    static constexpr float    kMaxAngle = 3.65f;

    int32_t  testRpm;     /* signed so we can detect under/overshoot */
    int16_t  testRpmStep; /* +N -> accel, -N -> decel */
};

#endif // SCREENVIEW_HPP
