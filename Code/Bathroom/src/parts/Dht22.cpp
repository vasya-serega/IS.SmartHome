#include "parts/Dht22.h"
#include <Logger.h>

void Dht22::init()
{
    _dht.begin();
}

float Dht22::getHumidity()
{
    sensors_event_t event;
    _dht.humidity().getEvent(&event);
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

float Dht22::getTemperature()
{
    sensors_event_t event;
    _dht.temperature().getEvent(&event);
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
