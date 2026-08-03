#pragma once

inline const char* Version = "1.0.0";
inline const char* ThresholdConfigFile = "threshold.dat";
inline const char* WiFiConfigFile = "wifi.dat";
inline const char* Hardware = "Rasberry Pi Pico W";
inline const int SerialBaudRate = 115200;
const short SensorHwdTimer = 0;
const short FanHwdTimer = 1;
const unsigned int FanInterval = 1000000;    // 1 second (timespan is a microsecond)
const unsigned int SensorInterval = 500000;    // 0.5 second (timespan is a microsecond)
const unsigned int WgIntervalMs = 1050;  // watchdog
const unsigned char DefaultHumidityThreshold = 60;