#include <Wire.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <MS5837.h>

const char* ssid = "BDCHSPheonix";  // Define your Access Point SSID here
const char* password = "7861105253";  // Define your Access Point password here

AsyncWebServer server(80);

RTC_DS1307 RTC;
MS5837 sensor;


void setup() {
  Serial.begin(9600);
  Wire.begin();
  sensor.setModel(MS5837::MS5837_02BA);
  sensor.init();
  sensor.setFluidDensity(997); // kg/m^3 (997 freshwater, 1029 for seawater)

  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  Serial.println("Time and date set");

  Serial.println("Starting Access Point");
  Serial.print("SSID: ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  WebSerial.begin(&server);
  server.begin();
}

void loop() {
  DateTime now = RTC.now();
  sensor.read();
  String datetime = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

  WebSerial.print("Team Number: 6189162");
  WebSerial.println();
  WebSerial.print("Time in UTC: ");
  WebSerial.println(datetime);
  WebSerial.print("Pressure: ");
  float pressure_mbar = sensor.pressure(); // Get pressure in mbar
  float pressure_kpa = pressure_mbar * 0.1; // Convert to kPa
  WebSerial.print(pressure_kpa, 2); // Display with 2 decimal places
  WebSerial.println(" kPa");
  WebSerial.print("Depth: "); 
  WebSerial.print(sensor.depth()); 
  WebSerial.println(" m");


  delay(1000);
}