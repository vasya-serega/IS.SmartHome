#pragma once

#include <DHT_U.h>
#include "pinout.h"

class Dht22
{
public:
    void init();
    float getHumidity();
    float getTemperature();

private:
    DHT_Unified _dht = DHT_Unified(HumiditySensorPin, DHT22);   // DHT 22 (AM2302)
};