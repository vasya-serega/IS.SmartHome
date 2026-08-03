#include "WiFi/Cyw43439Server.h"
#include <WiFiClient.h>
#include "HttpParser.h"

void Rp2040::Cyw43439Server::init(uint8_t serverIp[4], uint16_t port)
{
    IPAddress ip(serverIp);
    _server = std::make_shared<WiFiServer>(ip, port);

    _server->begin();
}

void Rp2040::Cyw43439Server::init(String mdnsHostname, String mdnsServiceName, uint16_t port)
{
    _server = std::make_shared<WiFiServer>(port);
    _server->begin();
}

String Rp2040::Cyw43439Server::getRawRequest(WiFiClient client)
{
    if (client.connected() == 0)
    {
        Serial.println("client is not connected");
        return "";
    }

    if (client.available() == 0)
    {
        Serial.println("client is not available");
        return "";
    }

    String request;
    bool currentLineIsBlank = true;

    while (true)
    {
        char c = client.read();
        request.concat(c);

        if (c == '\n' && currentLineIsBlank)
        {
            while (client.available())
            {
                char c = client.read();
                request.concat(c);
            }
            break;
        }
        else if (c == '\n')
        {
            currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
            currentLineIsBlank = false;
        }
    }
    return request;
}

void Rp2040::Cyw43439Server::sendResponse(WiFiClient client, HttpResponse response)
{
    client.println(response.toString());
}


void Rp2040::Cyw43439Server::handleRequest(IHttpHandler *handler)
{
    WiFiClient client = _server->accept();
    if (!client)
    {
        return;
    }

    String rawRequest = getRawRequest(client);
    if (handler != NULL)
    {
        HttpRequest request = Rp2040::HttpParser::getHttpRequest(rawRequest);
        HttpResponse response = handler->handle(request);
        sendResponse(client, response);

        delay(1);
        client.stop();
    }
}

void Rp2040::Cyw43439Server::handleRequest(HttpResponse (*callback)(HttpRequest))
{
    WiFiClient client = _server->accept();
    if (!client)
    {
        return;
    }

    String rawRequest = getRawRequest(client);
    if (callback != NULL)
    {
        HttpRequest request = Rp2040::HttpParser::getHttpRequest(rawRequest);
        HttpResponse response = callback(request);
        sendResponse(client, response);

        delay(1);
        client.stop();
    }
}