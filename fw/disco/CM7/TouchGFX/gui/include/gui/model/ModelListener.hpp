#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

#include "shared.h"

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    /* Called from Model::tick() once per UART telemetry sample. The
     * default is a no-op so screens that don't care about live data
     * (e.g. boot/diagnostic screens) don't have to override it. */
    virtual void notifySensor(const sensor_data_t& /*data*/) {}
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
