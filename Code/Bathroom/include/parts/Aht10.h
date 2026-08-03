#pragma once

#include <Adafruit_AHTX0.h>

class Aht10
{
public:
    void init();
    float getHumidity();
    float getTemperature();

private:
    Adafruit_AHTX0 _aht;
    Adafruit_Sensor *_humiditySensor = nullptr;
    Adafruit_Sensor *_temperatureSensor = nullptr;

    bool areSensorsReady();
};