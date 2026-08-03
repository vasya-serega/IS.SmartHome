#pragma once

#include "Configuration.h"
#include "Constants.h"
#include <RPi_Pico_TimerInterrupt.h>
#include "StateService.h"

class FanService
{
public:
    FanService(Configuration &config, StateService &stateService);
    void init();
    bool getFanState() const;
    void update();

private:
    Configuration &_config;
    StateService &_state;
    RPI_PICO_Timer _sensorTimer = RPI_PICO_Timer(SensorHwdTimer);
    static FanService *_instance;
    bool _fanState = false;

    void setFanState(bool state);
    bool timerHandler(struct repeating_timer *t);
    static bool timerHandlerWrapper(struct repeating_timer *t);
};