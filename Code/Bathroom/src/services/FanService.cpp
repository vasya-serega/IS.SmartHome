#include "../include/services/FanService.h"
#include "pinout.h"

FanService *FanService::_instance = nullptr;

FanService::FanService(Configuration &config, StateService &stateService) : _config(config), _state(stateService)
{
    _instance = this;
}

void FanService::init()
{
    gpio_set_input_enabled(RelayPin, true);

    _sensorTimer.attachInterruptInterval(FanInterval, timerHandlerWrapper);

    //Logger::notice("Threshold:", String(_config.getHumidityThreshold()).c_str());
    //_config.setHumidityThreshold(57);
}

bool FanService::getFanState() const
{
    return _fanState;
}

void FanService::update()
{
    auto humidityThreshold = _config.getHumidityThreshold();
    auto currentHumidity = _state.humidity();

    if (currentHumidity > humidityThreshold && !_fanState)
    {
        setFanState(true);
    }
    else if (currentHumidity <= humidityThreshold && _fanState)
    {
        setFanState(false);
    }
}

void FanService::setFanState(bool state)
{
    _fanState = state;
    gpio_put(RelayPin, state ? 1 : 0); // digitalWrite(RelayPin, HIGH or LOW);
    Logger::notice("Fan state changed to: ", state ? "ON" : "OFF");
}

bool FanService::timerHandler(repeating_timer *t)
{
    update();

    return true;
}

bool FanService::timerHandlerWrapper(repeating_timer *t)
{
    if (_instance)
    {
        return _instance->timerHandler(t);
    }

    return false;
}