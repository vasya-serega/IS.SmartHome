#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <memory>
#include <array>
#include "IWiFiHttpServer.h"

namespace Rp2040
{
    class WiFiHttpServer
    {
    public:
        static constexpr size_t MaxConcurrentClients = 4;

        void init(Rp2040::IWiFiHttpServer *handler, uint16_t port = 80);
        void loop();

    private:
        enum class State
        {
            Free, // slot unused, ready to accept a new client
            ReadingHeaders,
            ReadingBody,
            Processing
        };

        struct ClientSession
        {
            State state = State::Free;
            WiFiClient client;
            String headerBlock;
            String body;
            long contentLength = 0;
            bool currentLineIsBlank = true;
            unsigned long stateStartedAt = 0;
        };

        IWiFiHttpServer *_handler = nullptr;
        std::shared_ptr<WiFiServer> _server;
        std::array<ClientSession, MaxConcurrentClients> _sessions;

        static constexpr unsigned long HeaderTimeoutMs = 2000;
        static constexpr unsigned long BodyTimeoutMs = 5000;

        void tickAccept();
        void tickSession(ClientSession &session);
        void resetSession(ClientSession &session);

        void tickReadingHeaders(ClientSession &session);
        void tickReadingBody(ClientSession &session);
        void tickProcessing(ClientSession &session);

        long parseContentLength(const String &headerBlock);
        Rp2040::HttpRequest parseRequest(const String &headerBlock, const String &body);
        void sendResponse(WiFiClient &client, const Rp2040::HttpResponse &response);
    };
}