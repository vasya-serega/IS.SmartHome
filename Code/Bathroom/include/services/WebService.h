#pragma once

#include "../include/httpServer/WiFiHttpServer.h"
#include "Configuration.h"
#include "FanService.h"
#include "NetManager.h"

enum HtmlPageType
{
    ManagementPage,
    WiFiPage
};

class WebService : public Rp2040::IWiFiHttpServer
{
public:
    WebService(Configuration &config, FanService &fanService, StateService &stateService, NetManager &netManager);
    void init(uint16_t port = 80);
    void loop();
    Rp2040::HttpResponse handleHttpRequest(const Rp2040::HttpRequest &request) override;

private:
    Configuration &_config;
    FanService &_fanService;
    StateService &_stateService;
    NetManager &_netManager;
    Rp2040::WiFiHttpServer _httpServer;

    static std::map<String, String> buildHeaders(unsigned int contentLength);
    static std::map<String, String> buildJsonHeaders(unsigned int contentLength);
    Rp2040::HttpResponse handleGetMethods(const String &request);
    Rp2040::HttpResponse handlePostMethods(const String &methodName, std::map<String, String> headers, String body);
    static Rp2040::HttpResponse notFoundMethodError(const String &methodName);
    static Rp2040::HttpResponse notSupportedMethodError();
    static Rp2040::HttpResponse redirectResponse(const String &location);
    Rp2040::HttpResponse getDeviceState();
    Rp2040::HttpResponse getGeneralInfo();
    Rp2040::HttpResponse getHtmlPage(HtmlPageType pageType = HtmlPageType::ManagementPage);
    Rp2040::HttpResponse handleSetHumidityThreshold(std::map<String, String> headers, String body);
    Rp2040::HttpResponse handleSetWifi(std::map<String, String> headers, String body);
    String createManagementPage();
    String createWiFiPage();

    // Shared HTML building blocks (reused across pages)
    static String htmlPageOpen(const char *title);
    String htmlPageHeader();
    static String htmlPageClose(const String &bodyScript);
    static String htmlAttributeEscape(const String &value);
};