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
     * 0.3 bar oil-pressure threshold drives the dashboard's oil
     * warning lamp; the 1.8 bar threshold and battery voltage are
     * not yet wired to any widget. */
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
        /* Lamp lit when oil pressure is BELOW the 0.3 bar threshold. */
        view.led_set(screenView::Led::Oil, data.value == 0);
        break;

    case SENSOR_ID_BATTERY_VOLTAGE:
    case SENSOR_ID_OIL_PRESSURE_1_8_BAR:
    default:
        /* Not wired to a widget yet. */
        break;
    }
}
