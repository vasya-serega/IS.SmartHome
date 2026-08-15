#include <Arduino.h>
#include "Configuration.h"
#include "../include/services/NetManager.h"
#include "../include/services/WebService.h"

Configuration config;
NetManager netManager(config);
StateService stateService;
FanService fanService(config, stateService);
WebService webService(config, fanService, stateService, netManager);

void setup() {
  delay(5000);
  config.init();
  netManager.connect();
  stateService.init();
  fanService.init();
  webService.init(80); // Initialize the web service on port 80

  watchdog_enable(WgIntervalMs, 1);
}

void loop() {
  // StateService::timerHandler to get humidity and temperature is called by hardware timer
  // FanService::timerHandler to set fan state is called by hardware timer

  webService.loop(); // handle incoming HTTP requests
  watchdog_update();
}
