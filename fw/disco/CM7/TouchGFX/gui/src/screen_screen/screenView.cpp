#include <gui/screen_screen/screenView.hpp>

screenView::screenView()
    : testRpm(0),  testRpmStep(60),
      testFuel(kMinFuelLitres), testFuelStep(1),
      testTemp(kMinTempC),      testTempStep(1)
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
}
