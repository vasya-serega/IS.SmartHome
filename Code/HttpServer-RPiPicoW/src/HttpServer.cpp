#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpParser.h"
#include <WiFiClass.h>
#include <DNSServer.h>

using namespace Rp2040;

void Rp2040::HttpServer::init(uint8_t serverIp[4], uint16_t port)
{
    _server.init(serverIp, port);
    _isInitialized = true;
}

void Rp2040::HttpServer::init(String mdnsHostname, String mdnsServiceName, uint16_t port)
{
    _server.init(mdnsHostname, mdnsServiceName, port);
    _isInitialized = true;

    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    dnsServer.start(DNS_PORT, mdnsHostname, WiFi.localIP());

    // Critical: Keep processing DNS requests in the loop
    //dnsServer.processNextRequest();
}

void Rp2040::HttpServer::handleRequest(HttpResponse (*callback)(HttpRequest) = nullptr)
{
    if (!Rp2040::HttpServer::_isInitialized)
    {
        return; // throw exception?
    }

    _server.handleRequest(callback);
}

void Rp2040::HttpServer::handleRequest(IHttpHandler *handler)
{
    if (!Rp2040::HttpServer::_isInitialized)
    {
        return; // throw exception?
    }

    _server.handleRequest(handler);
}