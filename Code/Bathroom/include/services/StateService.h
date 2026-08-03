#pragma once

#include "Constants.h"
//#include "parts/Dht22.h"
#include "parts/Aht10.h"
#include "pinout.h"
#include <RPi_Pico_TimerInterrupt.h>

class StateService
{
public:
    StateService();
    void init();
    float humidity();
    float temperature();

private:
    //Dht22 _dht;
    Aht10 _dht;
    float _humidity;
    float _temperature;
    RPI_PICO_Timer _sensorTimer = RPI_PICO_Timer(SensorHwdTimer);
    static StateService *_instance;
    bool timerHandler(struct repeating_timer *t);
    static bool timerHandlerWrapper(struct repeating_timer *t);
};