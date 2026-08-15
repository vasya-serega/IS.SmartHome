#include <ArduinoJson.h>
#include "Constants.h"
#include "../include/services/WebService.h"

using namespace Rp2040;

WebService::WebService(Configuration &config, FanService &fanService, StateService &stateService, NetManager &netManager)
    : _config(config), _fanService(fanService), _stateService(stateService), _netManager(netManager)
{
}

void WebService::init(uint16_t port)
{
  _httpServer.init(this, port);
  Logger::verbose("WiFi HTTP server initialized");
}

void WebService::loop()
{
  _httpServer.loop();
}

HttpResponse WebService::handleHttpRequest(const HttpRequest &request)
{
  Logger::notice("WebService. handle. MethodName:", request.methodName.c_str());
    switch (request.methodType)
    {
    case HttpMethodType::GET:
        return handleGetMethods(request.methodName);
    case HttpMethodType::POST:
        return handlePostMethods(request.methodName, request.headers, request.body);
    default:
        return notSupportedMethodError();
    }
}

std::map<String, String> WebService::buildHeaders(unsigned int contentLength)
{
    std::map<String, String> headers;
    headers["Content-Type"] = "text/html";
    headers["Content-Length"] = String(contentLength);
    headers["Connection"] = "close";

    return headers;
}

std::map<String, String> WebService::buildJsonHeaders(unsigned int contentLength)
{
    std::map<String, String> headers;
    headers["Content-Type"] = "application/json";
    headers["Content-Length"] = String(contentLength);
    headers["Connection"] = "close";

    return headers;
}

Rp2040::HttpResponse WebService::notFoundMethodError(const String &methodName)
{
    Rp2040::HttpResponse response;
    response.code = 404;
    response.codeDescription = "Not Found";
    response.headers = buildHeaders(0);
    response.body = "";

    return response;
}

Rp2040::HttpResponse WebService::redirectResponse(const String &location)
{
    Rp2040::HttpResponse response;
    response.code = 302;
    response.codeDescription = "Found";
    response.headers = buildHeaders(0);
    response.headers["Location"] = location;
    response.body = "";

    return response;
}

HttpResponse WebService::handleGetMethods(const String &methodName)
{
  if (methodName == "")
  {
    if (!_netManager.isConnected())
    {
      return redirectResponse("/ConfigWiFi");
    }
    return getHtmlPage();
  }

  if (methodName == "ConfigWiFi")
  {
    return getHtmlPage(HtmlPageType::WiFiPage);
  }

  if (methodName == "CurrentState")
  {
    return getDeviceState();
  }

  if (methodName == "GeneralInfo")
  {
    return getGeneralInfo();
  }

  return notFoundMethodError(methodName);
}

HttpResponse WebService::handlePostMethods(const String &methodName, std::map<String, String> headers, String body)
{
  if (methodName == "SetHumidityThreshold")
  {
    return handleSetHumidityThreshold(headers, body);
  }

  if (methodName == "SetWifi")
  {
    return handleSetWifi(headers, body);
  }

  return notFoundMethodError(methodName);
}

HttpResponse WebService::notSupportedMethodError()
{
  HttpResponse response;
  response.code = 405;
  response.codeDescription = "Method Not Allowed";
  response.headers = buildHeaders(0);
  response.headers["Allow"] = "GET,POST";
  response.body = "";

  return response;
}

HttpResponse WebService::getDeviceState()
{
  JsonDocument jsonState;
  jsonState["IsFanTurnedOn"] = _fanService.getFanState();
  jsonState["Humidity"] = _stateService.humidity();
  jsonState["Temperature"] = _stateService.temperature();

  String body = "";
  serializeJson(jsonState, body);

  HttpResponse response;
  response.code = 200;
  response.codeDescription = "OK";
  response.headers = buildHeaders(body.length());
  response.body = body;

  Logger::notice("RemoteService. Current state was requested." /*, body.c_str()*/);

  return response;
}

HttpResponse WebService::getGeneralInfo()
{
  Logger::notice("RemoteService.", "General information was requested.");
  JsonDocument jsonState;
  jsonState["Hardware"] = Hardware;
  jsonState["FirmwareVersion"] = Version;
  jsonState["HumidityThreshold"] = _config.getHumidityThreshold();

  String body = "";
  serializeJson(jsonState, body);

  HttpResponse response;
  response.code = 200;
  response.codeDescription = "OK";
  response.headers = buildHeaders(body.length());
  response.body = body;

  return response;
}

HttpResponse WebService::getHtmlPage(HtmlPageType pageType)
{
  String body = pageType == HtmlPageType::ManagementPage ? 
    createManagementPage() : 
    createWiFiPage();

  HttpResponse response;
  response.code = 200;
  response.codeDescription = "OK";
  response.headers = buildHeaders(body.length());
  response.body = body;

  auto notification = pageType == HtmlPageType::ManagementPage ? 
    "Management page was requested. Sending resoponse..." : 
    "WiFi page was requested. Sending resoponse...";
  Logger::notice(notification);

  return response;
}

HttpResponse WebService::handleSetHumidityThreshold(std::map<String, String> headers, String body)
{
  Logger::verbose("handleSetHumidityThreshold:", body.c_str());
  JsonDocument state;
  deserializeJson(state, body);

  _config.setHumidityThreshold(state["SetHumidityThreshold"]);

  Logger::notice("HumidityThreshold is set to ", String(_config.getHumidityThreshold()).c_str());
  HttpResponse response;
  response.code = 204;
  response.codeDescription = "No Content";
  response.headers = buildHeaders(0);

  return response;
}

HttpResponse WebService::handleSetWifi(std::map<String, String> headers, String body)
{
  Logger::verbose("handleSetWifi:", body.c_str());
  JsonDocument state;
  deserializeJson(state, body);

  String ssid = state["SSID"];
  String password = state["Password"];

  _config.setWiFiConnectionData(ssid, password);

  Logger::notice("Attempting to connect to WiFi network:", ssid.c_str());
  bool connected = _netManager.tryReconnect();

  JsonDocument resultJson;
  resultJson["Connected"] = connected;
  if (!connected)
  {
    resultJson["Error"] = "Unable to connect. Check the password and try again.";
  }

  String resultBody = "";
  serializeJson(resultJson, resultBody);

  Logger::notice(connected ? "WiFi connection succeeded. SSID:" : "WiFi connection failed. SSID:", ssid.c_str());

  HttpResponse response;
  response.code = 200;
  response.codeDescription = "OK";
  response.headers = buildJsonHeaders(resultBody.length());
  response.body = resultBody;

  return response;
}

// ---------------------------------------------------------------------------
// Shared HTML building blocks
// ---------------------------------------------------------------------------

String WebService::htmlPageOpen(const char *title)
{
  String head;
  head.reserve(3300);

  head += F(
      "<!DOCTYPE html>"
      "<html lang='en'>"
      "<head>"
      "<meta charset='UTF-8'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "<title>");
  head += title;
  head += F(
      "</title>"
      "<style>"
      ":root{"
      "--bg:#0f172a;--card:#1e293b;--text:#e2e8f0;--muted:#94a3b8;"
      "--accent:#38bdf8;--good:#22c55e;--warn:#f59e0b;--field:#0b1220;"
      "}"
      "*{box-sizing:border-box;margin:0;padding:0;}"
      "body{"
      "font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;"
      "background:var(--bg);color:var(--text);"
      "min-height:100vh;padding:16px;"
      "display:flex;flex-direction:column;align-items:center;"
      "}"
      "header{text-align:center;margin:12px 0 24px;}"
      "header h1{font-size:1.6rem;font-weight:600;}"
      "header p{color:var(--muted);font-size:0.85rem;margin-top:4px;}"
      ".grid{"
      "display:grid;"
      "grid-template-columns:repeat(2,1fr);"
      "gap:14px;width:100%;max-width:640px;"
      "}"
      "@media (min-width:700px){"
      ".grid{grid-template-columns:repeat(4,1fr);max-width:860px;}"
      ".panel{max-width:860px;}"
      "}"
      ".card{"
      "background:var(--card);border-radius:14px;padding:18px;"
      "text-align:center;box-shadow:0 4px 12px rgba(0,0,0,0.25);"
      "}"
      ".card .label{color:var(--muted);font-size:0.8rem;text-transform:uppercase;letter-spacing:0.05em;}"
      ".card .sublabel{color:var(--muted);font-size:0.68rem;margin-top:2px;}"
      ".card .value{font-size:2rem;font-weight:700;margin-top:8px;}"
      ".card .unit{font-size:1rem;color:var(--muted);margin-left:2px;}"
      ".status{"
      "display:inline-block;padding:4px 14px;border-radius:999px;"
      "font-weight:600;font-size:1.1rem;margin-top:8px;"
      "}"
      ".status.on{background:rgba(34,197,94,0.15);color:var(--good);}"
      ".status.off{background:rgba(148,163,184,0.15);color:var(--muted);}"
      ".panel{"
      "background:var(--card);border-radius:14px;padding:18px;"
      "width:100%;max-width:640px;margin-top:14px;"
      "box-shadow:0 4px 12px rgba(0,0,0,0.25);"
      "}"
      ".panel .label{color:var(--muted);font-size:0.8rem;text-transform:uppercase;letter-spacing:0.05em;}"
      ".panel .sublabel{color:var(--muted);font-size:0.75rem;margin-top:4px;margin-bottom:12px;}"
      ".backlink{"
      "display:inline-block;color:var(--muted);font-size:0.8rem;"
      "text-decoration:none;margin-bottom:14px;"
      "}"
      ".backlink:hover{color:var(--accent);}"
      ".set-form{display:flex;gap:10px;flex-wrap:wrap;align-items:center;}"
      ".set-form input[type=range]{"
      "flex:1 1 140px;min-width:0;accent-color:var(--accent);"
      "height:6px;margin:auto 0;"
      "-webkit-appearance:none;appearance:none;background:#334155;border-radius:999px;"
      "}"
      ".set-form input[type=range]::-webkit-slider-thumb{"
      "-webkit-appearance:none;appearance:none;width:22px;height:22px;"
      "border-radius:50%;background:var(--accent);cursor:pointer;"
      "}"
      ".set-form input[type=number]{"
      "-webkit-appearance:none;-moz-appearance:textfield;appearance:none;"
      "flex:0 0 64px;width:64px;min-width:0;background:var(--field);color:var(--text);"
      "border:1px solid #334155;border-radius:10px;padding:8px 4px;font-size:1rem;"
      "text-align:center;"
      "}"
      ".set-form input[type=number]::-webkit-outer-spin-button,"
      ".set-form input[type=number]::-webkit-inner-spin-button{"
      "-webkit-appearance:none;margin:0;"
      "}"
      ".set-form button{"
      "flex:0 0 auto;background:var(--accent);color:#04202e;border:none;"
      "border-radius:10px;padding:9px 16px;font-size:0.95rem;font-weight:600;"
      "cursor:pointer;white-space:nowrap;"
      "}"
      ".set-form button:active{opacity:0.85;}"
      ".field-group{"
      "display:flex;flex-direction:column;gap:6px;margin-bottom:14px;text-align:left;"
      "}"
      ".field-group label{"
      "color:var(--muted);font-size:0.75rem;text-transform:uppercase;letter-spacing:0.05em;"
      "}"
      ".field-group input[type=text],"
      ".field-group input[type=password],"
      ".field-group select{"
      "background:var(--field);color:var(--text);border:1px solid #334155;"
      "border-radius:10px;padding:10px 12px;font-size:1rem;width:100%;"
      "-webkit-appearance:none;appearance:none;"
      "}"
      ".wifi-form button{"
      "background:var(--accent);color:#04202e;border:none;border-radius:10px;"
      "padding:11px 18px;font-size:1rem;font-weight:600;cursor:pointer;width:100%;"
      "}"
      ".wifi-form button:active{opacity:0.85;}"
      ".set-msg{font-size:0.8rem;margin-top:10px;min-height:1.1em;}"
      ".set-msg.ok{color:var(--good);}"
      ".set-msg.err{color:#f87171;}"
      "footer{margin-top:28px;color:var(--muted);font-size:0.75rem;text-align:center;}"
      "@media (max-width:400px){"
      ".card .value{font-size:1.6rem;}"
      "header h1{font-size:1.3rem;}"
      "}"
      "</style>"
      "</head>"
      "<body>");

  return head;
}

String WebService::htmlPageHeader()
{
  String header;
  header.reserve(160);

  header += F("<header><h1>");
  header += Hardware;
  header += F(" &mdash; Climate Control</h1><p>Firmware v");
  header += Version;
  header += F("</p></header>");

  return header;
}

String WebService::htmlPageClose(const String &bodyScript)
{
  String tail;
  tail.reserve(bodyScript.length() + 32);
  tail += bodyScript;
  tail += F("</body></html>");

  return tail;
}

String WebService::htmlAttributeEscape(const String &value)
{
  String escaped;
  escaped.reserve(value.length() + 8);

  for (unsigned int i = 0; i < value.length(); i++)
  {
    char c = value[i];
    switch (c)
    {
    case '&':
      escaped += F("&amp;");
      break;
    case '"':
      escaped += F("&quot;");
      break;
    case '\'':
      escaped += F("&#39;");
      break;
    case '<':
      escaped += F("&lt;");
      break;
    case '>':
      escaped += F("&gt;");
      break;
    default:
      escaped += c;
      break;
    }
  }

  return escaped;
}

// ---------------------------------------------------------------------------
// Management page
// ---------------------------------------------------------------------------

String WebService::createManagementPage()
{
  auto humidityThreshold = _config.getHumidityThreshold();
  auto isFanTurnedOn = _fanService.getFanState();
  auto humidity = _stateService.humidity();
  auto temperature = _stateService.temperature();

  const char *fanStatusText = isFanTurnedOn ? "ON" : "OFF";
  const char *fanStatusClass = isFanTurnedOn ? "on" : "off";

  String page;
  page.reserve(5504); // avoid repeated reallocations

  page += htmlPageOpen("Climate Control");
  page += htmlPageHeader();

  page += F("<a class='backlink' href='/ConfigWiFi'>Wi-Fi settings</a>");

  page += F("<div class='grid'>");

  // Temperature
  page += F("<div class='card'><div class='label'>Temperature</div>"
            "<div class='value'>");
  page += String(temperature, 1);
  page += F("<span class='unit'>&deg;C</span></div></div>");

  // Humidity
  page += F("<div class='card'><div class='label'>Humidity</div>"
            "<div class='value'>");
  page += String(humidity, 1);
  page += F("<span class='unit'>%</span></div></div>");

  // Humidity threshold
  page += F("<div class='card'><div class='label'>Humidity threshold</div>"
            "<div class='sublabel'>Fan turns on above this level</div>"
            "<div class='value'>");
  page += String(static_cast<unsigned int>(humidityThreshold));
  page += F("<span class='unit'>%</span></div></div>");

  // Fan status
  page += F("<div class='card'><div class='label'>Fan</div>"
            "<div class='status ");
  page += fanStatusClass;
  page += F("'>");
  page += fanStatusText;
  page += F("</div></div>");

  page += F("</div>"); // .grid

  // Set humidity threshold panel (slider + number input)
  page += F("<div class='panel'>"
            "<div class='label'>Set humidity threshold</div>"
            "<div class='sublabel'>Fan will switch on once humidity exceeds this value (min 15%)</div>"
            "<form class='set-form' id='thresholdForm'>"
            "<input type='range' id='thresholdSlider' min='15' max='100' step='1' value='");
  page += String(static_cast<unsigned int>(humidityThreshold));
  page += F("'>"
            "<input type='number' id='thresholdInput' name='SetHumidityThreshold' "
            "min='15' max='100' step='1' value='");
  page += String(static_cast<unsigned int>(humidityThreshold));
  page += F("' required>"
            "<button type='submit'>Set</button>"
            "</form>"
            "<div class='set-msg' id='thresholdMsg'></div>"
            "</div>");

  page += F("<footer>Auto-refreshes every 10s</footer>");

  String script;
  script.reserve(1536);
  script += F(
      "<script>"
      "setTimeout(()=>location.reload(),10000);"
      "var slider=document.getElementById('thresholdSlider');"
      "var input=document.getElementById('thresholdInput');"
      "slider.addEventListener('input',function(){ input.value=slider.value; });"
      "input.addEventListener('input',function(){ slider.value=input.value; });"
      "document.getElementById('thresholdForm').addEventListener('submit', function(e){"
      "e.preventDefault();"
      "var msg=document.getElementById('thresholdMsg');"
      "var val=parseInt(input.value, 10);"
      "if(isNaN(val)||val<15||val>100){"
      "msg.textContent='Enter a value between 15 and 100.';"
      "msg.className='set-msg err';"
      "return;"
      "}"
      "msg.textContent='Saving...';"
      "msg.className='set-msg';"
      "var controller=new AbortController();"
      "var timeoutId=setTimeout(function(){ controller.abort(); }, 5000);"
      "fetch('SetHumidityThreshold',{"
      "method:'POST',"
      "headers:{'Content-Type':'application/json'},"
      "body:JSON.stringify({SetHumidityThreshold:val}),"
      "signal:controller.signal"
      "}).then(function(r){"
      "clearTimeout(timeoutId);"
      "if(!r.ok) throw new Error('HTTP '+r.status);"
      "msg.textContent='Threshold updated.';"
      "msg.className='set-msg ok';"
      "}).catch(function(err){"
      "clearTimeout(timeoutId);"
      "msg.textContent='Failed to update threshold: '+(err.name==='AbortError'?'timed out':err.message);"
      "msg.className='set-msg err';"
      "});"
      "});"
      "</script>");

  page += htmlPageClose(script);

  return page;
}

// ---------------------------------------------------------------------------
// Wi-Fi configuration page
// ---------------------------------------------------------------------------

String WebService::createWiFiPage()
{
  String currentSsid = _config.getWiFiSsid();
  auto networks = _netManager.scanNetworks(); // blocking scan, runs on every page load

  String page;
  page.reserve(4352);

  page += htmlPageOpen("Wi-Fi Configuration");
  page += htmlPageHeader();

  page += F("<div class='panel'>"
            "<div class='label'>Wi-Fi configuration</div>"
            "<div class='sublabel'>Select the wireless network to connect this device to</div>"
            "<form class='wifi-form' id='wifiForm'>"
            "<div class='field-group'>"
            "<label for='ssidSelect'>Network name (SSID)</label>"
            "<select id='ssidSelect' name='SSID' required>");

  if (networks.empty())
  {
    page += F("<option value='' disabled selected>No networks found</option>");
  }
  else
  {
    bool hasSelectedMatch = false;
    for (const auto &network : networks)
    {
      if (network.isCurrentAp)
      {
        continue; // don't offer the device's own fallback AP as a target
      }

      String escapedSsid = htmlAttributeEscape(network.ssid);
      bool isSelected = (network.ssid == currentSsid);
      if (isSelected)
      {
        hasSelectedMatch = true;
      }

      page += F("<option value='");
      page += escapedSsid;
      page += F("'");
      if (isSelected)
      {
        page += F(" selected");
      }
      page += F(">");
      page += escapedSsid;
      page += network.isOpen ? F(" (open)") : F(" (secured)");
      page += F(" &middot; ");
      page += String(network.rssi);
      page += F(" dBm</option>");
    }

    if (!hasSelectedMatch && currentSsid.length() > 0)
    {
      // Configured network wasn't seen in this scan (out of range, hidden, etc.) —
      // still offer it so the form doesn't silently switch to a different network.
      String escapedCurrent = htmlAttributeEscape(currentSsid);
      page += F("<option value='");
      page += escapedCurrent;
      page += F("' selected>");
      page += escapedCurrent;
      page += F(" (currently configured, not in range)</option>");
    }
  }

  page += F("</select>"
            "</div>"
            "<div class='field-group'>"
            "<label for='passwordInput'>Password</label>"
            "<input type='password' id='passwordInput' name='Password' maxlength='63' "
            "autocomplete='off' placeholder='Leave blank for open networks'>"
            "</div>"
            "<button type='submit'>Save &amp; connect</button>"
            "</form>"
            "<div class='set-msg' id='wifiMsg'></div>"
            "<a id='dashboardLink' class='backlink' href='/' "
            "style='display:none;margin-top:10px;'>Go to dashboard &rarr;</a>"
            "</div>");

  String script;
  script.reserve(1536);
  script += F(
      "<script>"
      "document.getElementById('wifiForm').addEventListener('submit', function(e){"
      "e.preventDefault();"
      "var msg=document.getElementById('wifiMsg');"
      "var link=document.getElementById('dashboardLink');"
      "link.style.display='none';"
      "var select=document.getElementById('ssidSelect');"
      "var ssid=select.value;"
      "var password=document.getElementById('passwordInput').value;"
      "if(!ssid){"
      "msg.textContent='Please select a network.';"
      "msg.className='set-msg err';"
      "return;"
      "}"
      "msg.textContent='Connecting...';"
      "msg.className='set-msg';"
      "var controller=new AbortController();"
      "var timeoutId=setTimeout(function(){ controller.abort(); }, 20000);"
      "fetch('SetWifi',{"
      "method:'POST',"
      "headers:{'Content-Type':'application/json'},"
      "body:JSON.stringify({SSID:ssid,Password:password}),"
      "signal:controller.signal"
      "}).then(function(r){"
      "clearTimeout(timeoutId);"
      "if(!r.ok) throw new Error('HTTP '+r.status);"
      "return r.json();"
      "}).then(function(data){"
      "if(data.Connected){"
      "msg.textContent='Connected successfully.';"
      "msg.className='set-msg ok';"
      "link.style.display='inline-block';"
      "}else{"
      "msg.textContent=data.Error||'Failed to connect. Please try again.';"
      "msg.className='set-msg err';"
      "}"
      "}).catch(function(err){"
      "clearTimeout(timeoutId);"
      "msg.textContent='Failed to save Wi-Fi settings: '+(err.name==='AbortError'?'timed out':err.message);"
      "msg.className='set-msg err';"
      "});"
      "});"
      "</script>");

  page += htmlPageClose(script);

  return page;
}