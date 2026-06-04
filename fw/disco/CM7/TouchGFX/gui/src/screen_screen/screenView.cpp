#include <gui/screen_screen/screenView.hpp>

screenView::screenView()
    : testRpm(0), testRpmStep(60)
{

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
}
