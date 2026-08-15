#pragma once

#include <vector>
#include <Arduino.h>

struct WiFiNetworkInfo
{
    String ssid;
    int32_t rssi;
    bool isOpen;
    bool isCurrentAp;
};

class NetManager
{
    public:
    NetManager(Configuration &config);
    void connect();
    bool isConnected();
    bool tryReconnect();
    bool tryConnectWithCredentials(const String &ssid, const String &password);
    std::vector<WiFiNetworkInfo> scanNetworks();
private:
    Configuration &_config;

    void startAccessPoint();
};