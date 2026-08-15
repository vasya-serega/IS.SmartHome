#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

namespace Rp2040
{
    class IWiFiHttpServer
    {
    public:
        virtual ~IWiFiHttpServer() = default;
        virtual Rp2040::HttpResponse handleHttpRequest(const Rp2040::HttpRequest &request) = 0;
    };
}