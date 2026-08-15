#include "../include/httpServer/WiFiHttpServer.h"
#include <Logger.h>

using namespace Rp2040;

void WiFiHttpServer::init(IWiFiHttpServer *handler, uint16_t port)
{
    _handler = handler;
    _server = std::make_shared<WiFiServer>(port);
    _server->begin();
    Logger::notice("WiFiHttpServer started on port:", String(port).c_str());
}

void WiFiHttpServer::loop()
{
    if (!_server || !_handler)
    {
        return;
    }

    tickAccept();

    for (auto &session : _sessions)
    {
        if (session.state != State::Free)
        {
            tickSession(session);
        }
    }
}

void WiFiHttpServer::tickAccept()
{
    // Only try to accept if there's an actual pending connection AND a free slot;
    // otherwise leave it queued at the TCP level for a future loop() pass.
    WiFiClient newClient = _server->accept();
    if (!newClient)
    {
        return;
    }

    for (auto &session : _sessions)
    {
        if (session.state == State::Free)
        {
            session.client = newClient;
            session.headerBlock = "";
            session.body = "";
            session.contentLength = 0;
            session.currentLineIsBlank = true;
            session.stateStartedAt = millis();
            session.state = State::ReadingHeaders;
            return;
        }
    }

    // No free slot: pool is at capacity, politely reject this connection
    // rather than silently dropping it or blocking other clients.
    Logger::verbose("Client pool full, rejecting new connection.");
    newClient.stop();
}

void WiFiHttpServer::tickSession(ClientSession &session)
{
    switch (session.state)
    {
    case State::ReadingHeaders:
        tickReadingHeaders(session);
        break;
    case State::ReadingBody:
        tickReadingBody(session);
        break;
    case State::Processing:
        tickProcessing(session);
        break;
    default:
        break;
    }
}

void WiFiHttpServer::resetSession(ClientSession &session)
{
    if (session.client)
    {
        session.client.stop();
    }
    session.client = WiFiClient();
    session.headerBlock = "";
    session.body = "";
    session.contentLength = 0;
    session.currentLineIsBlank = true;
    session.state = State::Free;
}

void WiFiHttpServer::tickReadingHeaders(ClientSession &session)
{
    if (!session.client.connected() && session.client.available() == 0)
    {
        Logger::verbose("Client disconnected while reading headers.");
        resetSession(session);
        return;
    }

    while (session.client.available() > 0)
    {
        char c = session.client.read();
        session.headerBlock.concat(c);

        if (c == '\n' && session.currentLineIsBlank)
        {
            long contentLength = parseContentLength(session.headerBlock);
            session.contentLength = contentLength;
            session.stateStartedAt = millis();
            session.state = (contentLength > 0) ? State::ReadingBody : State::Processing;
            return;
        }
        else if (c == '\n')
        {
            session.currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
            session.currentLineIsBlank = false;
        }
    }

    if (millis() - session.stateStartedAt > HeaderTimeoutMs)
    {
        Logger::verbose("Timed out reading headers.");
        resetSession(session);
    }
}

void WiFiHttpServer::tickReadingBody(ClientSession &session)
{
    if (!session.client.connected() && session.client.available() == 0)
    {
        Logger::verbose("Client disconnected while reading body.");
        resetSession(session);
        return;
    }

    while (session.client.available() > 0 && (long)session.body.length() < session.contentLength)
    {
        char c = session.client.read();
        session.body.concat(c);
    }

    if ((long)session.body.length() >= session.contentLength)
    {
        session.state = State::Processing;
        return;
    }

    if (millis() - session.stateStartedAt > BodyTimeoutMs)
    {
        Logger::verbose("Timed out reading body. Got bytes:", String(session.body.length()).c_str());
        resetSession(session);
    }
}

void WiFiHttpServer::tickProcessing(ClientSession &session)
{
    HttpRequest request = parseRequest(session.headerBlock, session.body);
    Logger::notice("Received request:", (request.methodName.length() ? request.methodName.c_str() : "/"));

    Rp2040::HttpResponse response = _handler->handleHttpRequest(request);
    sendResponse(session.client, response);

    resetSession(session);
}

long WiFiHttpServer::parseContentLength(const String &headerBlock)
{
    int idx = headerBlock.indexOf("Content-Length:");
    if (idx == -1)
    {
        idx = headerBlock.indexOf("content-length:");
    }
    if (idx == -1)
    {
        return 0;
    }

    int valueStart = idx + strlen("Content-Length:");
    int lineEnd = headerBlock.indexOf('\r', valueStart);
    if (lineEnd == -1)
    {
        lineEnd = headerBlock.indexOf('\n', valueStart);
    }
    if (lineEnd == -1)
    {
        lineEnd = headerBlock.length();
    }

    String valueStr = headerBlock.substring(valueStart, lineEnd);
    valueStr.trim();
    return valueStr.toInt();
}

HttpRequest WiFiHttpServer::parseRequest(const String &headerBlock, const String &body)
{
    HttpRequest request;
    request.body = body;

    int lineEnd = headerBlock.indexOf('\n');
    String requestLine = lineEnd == -1 ? headerBlock : headerBlock.substring(0, lineEnd);
    requestLine.trim();

    int firstSpace = requestLine.indexOf(' ');
    int secondSpace = requestLine.indexOf(' ', firstSpace + 1);

    if (firstSpace == -1 || secondSpace == -1)
    {
        request.methodType = HttpMethodType::UNKNOWN;
        return request;
    }

    String methodStr = requestLine.substring(0, firstSpace);
    String path = requestLine.substring(firstSpace + 1, secondSpace);

    if (methodStr == "GET")
    {
        request.methodType = HttpMethodType::GET;
    }
    else if (methodStr == "POST")
    {
        request.methodType = HttpMethodType::POST;
    }
    else
    {
        request.methodType = HttpMethodType::UNKNOWN;
    }

    if (path.startsWith("/"))
    {
        path = path.substring(1);
    }
    request.methodName = path;

    int headersStart = lineEnd + 1;
    int cursor = headersStart;
    while (cursor < (int)headerBlock.length())
    {
        int nextLineEnd = headerBlock.indexOf('\n', cursor);
        if (nextLineEnd == -1)
        {
            nextLineEnd = headerBlock.length();
        }

        String line = headerBlock.substring(cursor, nextLineEnd);
        line.trim();

        if (line.length() == 0)
        {
            break;
        }

        int colonIdx = line.indexOf(':');
        if (colonIdx != -1)
        {
            String key = line.substring(0, colonIdx);
            String value = line.substring(colonIdx + 1);
            key.trim();
            value.trim();
            request.headers[key] = value;
        }

        cursor = nextLineEnd + 1;
    }

    return request;
}

void WiFiHttpServer::sendResponse(WiFiClient &client, const Rp2040::HttpResponse &response)
{
    client.println(response.toString());
}