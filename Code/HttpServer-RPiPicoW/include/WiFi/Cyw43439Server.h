#pragma once

#include "IHttpHandler.h"
#include "BaseHttpServer.h"
#include <WiFiServer.h>
#include <memory>

namespace Rp2040
{
    class Cyw43439Server// : public BaseHttpServer
    {
    public:
        //void init(uint8_t serverIp[4], uint16_t port = 80) override;
        //void init(String mdnsHostname, String mdnsServiceName, uint16_t port = 80);
        //void handleRequest(HttpResponse (*callback)(HttpRequest)) override;
        //void handleRequest(IHttpHandler *handler) override;
        void init(uint16_t port = 80);
        void handleRequest(HttpResponse (*callback)(HttpRequest));
        void handleRequest(IHttpHandler *handler);

    private:
        std::shared_ptr<WiFiServer> _server;
        String getRawRequest(WiFiClient client);
        void sendResponse(WiFiClient client, HttpResponse response);
    };

}