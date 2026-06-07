#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

#include "SensorDataPublisher.h"

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
    /* Drain everything the UART receiver pushed since the last frame.
     * The publisher is non-blocking and bounded, so this loop always
     * terminates within at most SENSOR_DATA_QUEUE_LENGTH iterations. */
    sensor_data_t sample;
    while (SensorDataPublisherTryReceive(&sample))
    {
        if (modelListener != 0)
        {
            modelListener->notifySensor(sample);
        }
    }
}
