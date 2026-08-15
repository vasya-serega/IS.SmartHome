#include "../include/httpServer/HttpResponse.h"

using namespace Rp2040;

String HttpResponse::toString() const
{
    const char *endLine = "\r\n";
    String reply = "HTTP/1.1 ";
    reply.concat(code);
    reply.concat(' ');
    reply.concat(codeDescription);
    reply.concat(endLine);

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        reply.concat(it->first);
        reply.concat(": ");
        reply.concat(it->second);
        reply.concat(endLine);
    }
    reply.concat(endLine);
    reply.concat(body);

    return reply;
}