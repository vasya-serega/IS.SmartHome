#include <ArduinoJson.h>
#include "Configuration.h"
#include "Constants.h"

uint8_t Configuration::getHumidityThreshold() const
{
    return _humidityThreshold;
}

String Configuration::getWiFiSsid() const
{
    return _wifiSsid;
}

String Configuration::getWiFiPassword() const
{
    return _wifiPassword;
}

void Configuration::init()
{
    Serial.begin(SerialBaudRate);
    Logger::setLogLevel(Logger::Level::VERBOSE);
    Logger::notice("Initialization...");

    if (!LittleFS.begin())
    {
        Logger::error("LittleFS is not ready. Check parameter 'board_build.filesystem_size' in .ini file");
        return;
    }

    initThresholdFileAndValue();
    initWiFiFileAndValue();

    Logger::notice("Read initial value of humidity threshold:", String(_humidityThreshold).c_str());
}

const String Configuration::serverIpString()
{
    String serverBaseUrl = "http://";
    serverBaseUrl += String(Configuration::serverIp[0]);
    serverBaseUrl += ".";
    serverBaseUrl += String(Configuration::serverIp[1]);
    serverBaseUrl += ".";
    serverBaseUrl += String(Configuration::serverIp[2]);
    serverBaseUrl += ".";
    serverBaseUrl += String(Configuration::serverIp[3]);

    return serverBaseUrl;
}

void Configuration::setHumidityThreshold(uint8_t threshold)
{
    if (_humidityThreshold == threshold)
    {
        return;
    }

    _humidityThreshold = threshold;
    if (_tresholdFile)
    {
        _tresholdFile.seek(0);
        _tresholdFile.write(_humidityThreshold);
        _tresholdFile.close();
        _tresholdFile = LittleFS.open(ThresholdConfigFile, "r+");

        Logger::notice("Humidity threshold was updated. New value:", String(_humidityThreshold).c_str());
    }
}

void Configuration::setWiFiConnectionData(const String &ssid, const String &password)
{
    if (_wifiSsid == ssid && _wifiPassword == password)
    {
        return;
    }

    _wifiSsid = ssid;
    _wifiPassword = password;

    if (_wifiFile)
    {
        _wifiFile.seek(0);
        JsonDocument jsonStruct;
        jsonStruct["SSID"] = _wifiSsid;
        jsonStruct["Password"] = _wifiPassword;
        String jsonData = "";
        serializeJson(jsonStruct, jsonData);
        _wifiFile.write(jsonData.c_str());
        _wifiFile.close();
        _wifiFile = LittleFS.open(WiFiConfigFile, "r+");

        Logger::notice("WiFi connection data was updated. New SSID:", _wifiSsid.c_str());
    }
}

void Configuration::initThresholdFileAndValue()
{
    if (!LittleFS.exists(ThresholdConfigFile))
    {
        Logger::notice("Threshold configuration file does not exist, creating a new one", ThresholdConfigFile);
        _tresholdFile = LittleFS.open(ThresholdConfigFile, "w+");
        _tresholdFile.write(_humidityThreshold);
    }
    _tresholdFile = LittleFS.open(ThresholdConfigFile, "r+"); // a
    _tresholdFile.readBytes(reinterpret_cast<char *>(&_humidityThreshold), sizeof(_humidityThreshold));
}

void Configuration::initWiFiFileAndValue()
{
    if (!LittleFS.exists(WiFiConfigFile))
    {
        Logger::notice("WiFi configuration file does not exist, creating a new one", WiFiConfigFile);
        _wifiFile = LittleFS.open(WiFiConfigFile, "w+");

        JsonDocument jsonStruct;
        jsonStruct["SSID"] = _wifiSsid;
        jsonStruct["Password"] = _wifiPassword;
        String jsonData = "";
        serializeJson(jsonStruct, jsonData);
        _wifiFile.write(jsonData.c_str());
    }
    _wifiFile = LittleFS.open(WiFiConfigFile, "r+"); // a
    auto data = _wifiFile.readString();
    JsonDocument wifiConfig;
    deserializeJson(wifiConfig, data);
    _wifiSsid = wifiConfig["SSID"].as<String>();
    _wifiPassword = wifiConfig["Password"].as<String>();
}