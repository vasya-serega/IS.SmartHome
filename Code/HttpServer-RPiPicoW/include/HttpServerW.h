#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WiFi/Cyw43439Server.h"
#include <memory>

namespace Rp2040
{
    class HttpServerW
    {
    private:
        bool _isInitialized = false;
        Cyw43439Server _server;

    public:
        // void init(uint8_t serverIp[4], uint16_t port = 80);
        // void init(String mdnsHostname, String mdnsServiceName, uint16_t port = 80);
        void init(uint16_t port = 80);
        void handleRequest(HttpResponse (*callback)(HttpRequest));
        void handleRequest(IHttpHandler *handler);
    };
}
