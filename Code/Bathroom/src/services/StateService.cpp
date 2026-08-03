#include "../include/services/StateService.h"

StateService *StateService::_instance = nullptr;

StateService::StateService()
{
    _instance = this;
    _humidity = 0;
    _temperature = -80;
}

void StateService::init()
{
    _dht.init();
    _sensorTimer.attachInterruptInterval(SensorInterval, timerHandlerWrapper);
}

float StateService::humidity()
{
    return _humidity;
}

float StateService::temperature()
{
    return _temperature;
}

bool StateService::timerHandler(struct repeating_timer *t)
{
    _humidity = _dht.getHumidity();
    _temperature = _dht.getTemperature();

    return true;
}

bool StateService::timerHandlerWrapper(struct repeating_timer *t)
{
    if (_instance)
    {
        return _instance->timerHandler(t);
    }

    return false;
}


