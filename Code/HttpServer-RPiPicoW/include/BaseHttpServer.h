#pragma once

#include "IHttpHandler.h"

namespace Rp2040
{
    class BaseHttpServer
    {
    public:
        virtual void init(uint8_t serverIp[4], uint16_t port = 80) = 0;
        virtual void handleRequest(HttpResponse (*callback)(HttpRequest)) = 0;
        virtual void handleRequest(IHttpHandler *handler) = 0;
    };
}
