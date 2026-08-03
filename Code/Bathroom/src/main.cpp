#include <Arduino.h>
#include "Configuration.h"
#include <services/WebService.h>
#include <EthernetUdp.h>
#include <ArduinoMDNS.h>

EthernetUDP udp;
MDNS mdns(udp);

Configuration config;
StateService stateService;
FanService fanService(config, stateService);
WebService webService(config, fanService, stateService);

void setup() {
  delay(5000);
  stateService.init();
  fanService.init();
  webService.init();

  mdns.begin(Ethernet.localIP(), config.mdnsHostname);

  mdns.addServiceRecord(
    "IS Bathroom._http",   // service instance name
    80,                       // your existing server port
    MDNSServiceTCP
);
  watchdog_enable(WgIntervalMs, 1);
}

void loop() {
  // StateService::timerHandler to get humidity and temperature is called by hardware timer
  // FanService::timerHandler to set fan state is called by hardware timer

  mdns.run();

  webService.loop(); // handle incoming HTTP requests
  watchdog_update();
}
