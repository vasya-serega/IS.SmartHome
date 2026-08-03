#include "parts/Aht10.h"
#include <Logger.h>

void Aht10::init()
{
    if (_aht.begin())
    {
        _humiditySensor = _aht.getHumiditySensor();
        _temperatureSensor = _aht.getTemperatureSensor();
    }
}

float Aht10::getHumidity()
{
    if (!areSensorsReady())
    {
        return -1.0f;
    }

    sensors_event_t event;
    _humiditySensor->getEvent(&event);

    if (isnan(event.relative_humidity))
    {
        Logger::error("Error reading humidity from DHT sensor");
    }
    else
    {
        Logger::verbose("Current humidity: ", String(String(event.relative_humidity) + "%").c_str());
    }

    return event.relative_humidity;
}

float Aht10::getTemperature()
{
    if (!areSensorsReady())
    {
        return -1.0f;
    }
    sensors_event_t event;
    _temperatureSensor->getEvent(&event);
     if (isnan(event.temperature))
    {
        Logger::error("Error reading temperature from DHT sensor");
    }
    else
    {
        Logger::verbose("Current temperature: ", String(String(event.temperature) + "°C").c_str());
    }

    return event.temperature;
}

bool Aht10::areSensorsReady()
{
    if (_humiditySensor == nullptr || _temperatureSensor == nullptr)
    {
        //Logger::warning("Sensors are not ready yet");
        _humiditySensor = _aht.getHumiditySensor();
        _temperatureSensor = _aht.getTemperatureSensor();

        return false;
    }

    return true;
}