#ifndef SCREENPRESENTER_HPP
#define SCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class screenView;

class screenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    screenPresenter(screenView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    /* Drained from Model::tick() once per UART telemetry sample.
     * Dispatches by sensor_id_t onto the matching screenView setter
     * (gauges) or LED. */
    virtual void notifySensor(const sensor_data_t& data);

    virtual ~screenPresenter() {}

private:
    screenPresenter();

    /* Recompute the oil-pressure warning lamp from the latched
     * 0.3 bar / 1.8 bar threshold signals. The lamp is OFF only when
     * BOTH thresholds are high (== 1); any other combination lights
     * it. Default-initialised to false -> lamp is lit until both are received. */
    void updateOilLamp();

    screenView& view;

    bool oil03BarOk = false; /* last sample of SENSOR_ID_OIL_PRESSURE_0_3_BAR */
    bool oil18BarOk = false; /* last sample of SENSOR_ID_OIL_PRESSURE_1_8_BAR */
};

#endif // SCREENPRESENTER_HPP
