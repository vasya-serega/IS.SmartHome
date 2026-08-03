#pragma once

#include <LittleFS.h>
#include <Logger.h>
#include "Constants.h"

class Configuration
{
    public:
        static constexpr uint8_t serverIp[4] = {192, 168, 198, 127};
        static constexpr const char* mdnsHostname = "is-bathroom";

        uint8_t getHumidityThreshold() const;
        String getWiFiSsid() const;
        String getWiFiPassword() const;
        void init();
        static const String serverIpString();
        void setHumidityThreshold(uint8_t threshold);
        void setWiFiConnectionData(const String &ssid, const String &password);

    private:
        File _tresholdFile;
        File _wifiFile;
        uint8_t _humidityThreshold = DefaultHumidityThreshold;
        String _wifiSsid = "";
        String _wifiPassword = "";
        void initThresholdFileAndValue();
        void initWiFiFileAndValue();
};