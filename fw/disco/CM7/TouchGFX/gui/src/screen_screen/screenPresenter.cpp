#include <gui/screen_screen/screenView.hpp>
#include <gui/screen_screen/screenPresenter.hpp>

screenPresenter::screenPresenter(screenView& v)
    : view(v)
{

}

void screenPresenter::activate()
{

}

void screenPresenter::deactivate()
{

}

void screenPresenter::notifySensor(const sensor_data_t& data)
{
    int16_t tempC;
    uint16_t litres;
    /* Map the wire-protocol sensor_id_t onto the screenView setters.
     * Binary signals are interpreted as "lit when value != 0". The
     * oil warning lamp is driven by the COMBINATION of the 0.3 bar
     * and 1.8 bar threshold signals (see updateOilLamp). The battery
     * lamp lights when the measured voltage is below 13.00 V; the
     * wire value is in 10 mV units (1300 == 13.00 V). */
    switch (data.id)
    {
    case SENSOR_ID_COOLANT_TEMPERATURE:
        // scale to deg C from 0.1 deg C
        tempC = (data.value + 5) / 10;
        view.setTemperature(tempC);
        break;

    case SENSOR_ID_MOTOR_RPM:
        view.setRPM(data.value < 0 ? 0u
                                   : static_cast<uint16_t>(data.value));
        break;

    case SENSOR_ID_FUEL_LEVEL:
        // scale to litres from 0.1 litres
        litres = (data.value + 5) / 10;
        view.setFuel(litres < 0 ? 0u
                                 : static_cast<uint16_t>(litres));
        break;

    case SENSOR_ID_TURN_SIGNAL:
        view.led_set(screenView::Led::Indicator, data.value != 0);
        break;

    case SENSOR_ID_HIGH_BEAM:
        view.led_set(screenView::Led::HighBeam, data.value != 0);
        break;

    case SENSOR_ID_OIL_PRESSURE_0_3_BAR:
        oil03BarOk = (data.value != 0);
        updateOilLamp();
        break;

    case SENSOR_ID_OIL_PRESSURE_1_8_BAR:
        oil18BarOk = (data.value != 0);
        updateOilLamp();
        break;

    case SENSOR_ID_BATTERY_VOLTAGE:
        /* value is in 10 mV units; threshold 13.00 V == 1300. */
        /* TODO: this signal should actually be compared to "Klemme 61" from the alternator.*/
        view.led_set(screenView::Led::Battery, data.value < 1300);
        break;

    default:
        /* Not wired to a widget yet. */
        break;
    }
}

void screenPresenter::updateOilLamp()
{
    /* Lamp OFF only when both threshold signals are asserted (oil
     * pressure healthy at both check points). Any other combination
     * -- low pressure, sensor disagreement, or one signal not yet
     * received -- lights the warning. */
    // for now, only the oil03BarOk input is routed. Should be:
    // const bool warn = !(oil03BarOk && oil18BarOk);
    const bool warn = !oil03BarOk;
    view.led_set(screenView::Led::Oil, warn);
}
