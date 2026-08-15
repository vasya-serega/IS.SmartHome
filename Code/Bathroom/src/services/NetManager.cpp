#include "Configuration.h"
#include <Logger.h>
#include <WiFi.h>
#include <algorithm>
#include "../include/services/NetManager.h"

NetManager::NetManager(Configuration &config)
    : _config(config)
{
}

void NetManager::connect()
{
    WiFi.mode(WIFI_AP_STA);
    auto ssid = _config.getWiFiSsid();
    auto password = _config.getWiFiPassword();
    if (ssid.isEmpty())
    {
        Logger::warning("WiFi SSID is empty. Starting Access Point mode.");
        startAccessPoint();
        return;
    }

    Logger::notice("Connecting to WiFi network: ", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    const unsigned long connectTimeoutMs = 15000;
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < connectTimeoutMs)
    {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Logger::notice("Connected to WiFi network: ", ssid.c_str());
        Logger::notice("Pico W IP Address: ", WiFi.localIP().toString().c_str());
    }
    else
    {
        Logger::warning("Unable to connect to WiFi network: ", ssid.c_str());
        startAccessPoint();
    }
}

bool NetManager::tryReconnect()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    Logger::notice("Trying to reconnect to WiFi network...");
    WiFi.disconnect();

    auto ssid = _config.getWiFiSsid();
    auto password = _config.getWiFiPassword();

    if (ssid.isEmpty())
    {
        Logger::warning("WiFi SSID is empty. Cannot connect.");
        return false;
    }

    WiFi.begin(ssid.c_str(), password.c_str());

    const unsigned long connectTimeoutMs = 15000;
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < connectTimeoutMs)
    {
        delay(250);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Logger::notice("Reconnected to WiFi network: ", ssid.c_str());
        Logger::notice("Pico W IP Address: ", WiFi.localIP().toString().c_str());
        return true;
    }
    else
    {
        Logger::error("Failed to reconnect to WiFi network: ", ssid.c_str());
        return false;
    }
}

bool NetManager::isConnected()
{
    // Reflects the outer (station) WiFi connection only — the local fallback
    // Access Point running alongside it does not count as "connected" here.
    return WiFi.status() == WL_CONNECTED;
}

std::vector<WiFiNetworkInfo> NetManager::scanNetworks()
{
    std::vector<WiFiNetworkInfo> networks;

    Logger::notice("Scanning for WiFi networks...");
    int foundCount = WiFi.scanNetworks();

    if (foundCount <= 0)
    {
        Logger::warning("No WiFi networks found (or scan failed). Result code:", String(foundCount).c_str());
        WiFi.scanDelete();
        return networks;
    }

    networks.reserve(foundCount);

    for (int i = 0; i < foundCount; i++)
    {
        String ssid = WiFi.SSID(i);
        if (ssid.isEmpty())
        {
            continue;
        }

        WiFiNetworkInfo info;
        info.ssid = ssid;
        info.rssi = WiFi.RSSI(i);
        info.isOpen = (WiFi.encryptionType(i) == ENC_TYPE_NONE);
        info.isCurrentAp = (ssid == String(_config.localSsid));

        networks.push_back(info);
    }

    WiFi.scanDelete();

    std::sort(networks.begin(), networks.end(), [](const WiFiNetworkInfo &a, const WiFiNetworkInfo &b) {
        if (a.ssid != b.ssid)
        {
            return a.ssid < b.ssid;
        }
        return a.rssi > b.rssi;
    });

    std::vector<WiFiNetworkInfo> deduped;
    deduped.reserve(networks.size());
    for (auto &net : networks)
    {
        if (deduped.empty() || deduped.back().ssid != net.ssid)
        {
            deduped.push_back(net);
        }
    }

    std::sort(deduped.begin(), deduped.end(), [](const WiFiNetworkInfo &a, const WiFiNetworkInfo &b) {
        return a.rssi > b.rssi;
    });

    Logger::notice("WiFi scan complete. Networks found:", String(deduped.size()).c_str());

    return deduped;
}

void NetManager::startAccessPoint()
{
    Logger::notice("Starting Access Point mode with SSID: ", _config.localSsid);

    WiFi.softAPConfig(IPAddress(_config.localServerIp[0], _config.localServerIp[1], _config.localServerIp[2], _config.localServerIp[3]),
                      IPAddress(_config.localServerIp[0], _config.localServerIp[1], _config.localServerIp[2], _config.localServerIp[3]),
                      IPAddress(255, 255, 255, 0));
    bool success = WiFi.softAP(_config.localSsid, _config.localPassword);

    if (success)
    {
        Logger::notice("Access Point successfully started.");
        Logger::notice("SSID: ", _config.localSsid);
        Logger::notice("Pico W IP Address: ", WiFi.softAPIP().toString().c_str());
    }
    else
    {
        Logger::error("Failed to start Access Point.");
    }
}