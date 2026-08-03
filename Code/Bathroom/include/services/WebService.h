#pragma once

#include <HttpServer.h>
#include "Configuration.h"
#include "FanService.h"

enum HtmlPageType
{
    ManagementPage,
    WiFiPage
};

class WebService : public Rp2040::IHttpHandler
{
public:
    WebService(Configuration &config, FanService &fanService, StateService &stateService);
    void init();
    void loop();
    Rp2040::HttpResponse handle(const Rp2040::HttpRequest &request) override;

private:
    Configuration &_config;
    FanService &_fanService;
    StateService &_stateService;
    Rp2040::HttpServer _httpServer = Rp2040::HttpServer(Rp2040::DeviceModel::W5500EvbPico);

    Rp2040::HttpResponse HandleGetMethods(String request);
    Rp2040::HttpResponse HandlePostMethods(String methodName, std::map<String, String> headers, String body);
    Rp2040::HttpResponse NotSupportedMethodError();
    static std::map<String, String> GetHeaders(unsigned int contentLenght);
    Rp2040::HttpResponse getDeviceState();
    Rp2040::HttpResponse getGeneralInfo();
    Rp2040::HttpResponse getHtmlPage(HtmlPageType pageType = HtmlPageType::ManagementPage);
    static Rp2040::HttpResponse notFoundMethodError(String methodName);
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