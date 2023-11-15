#include <Wire.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <MS5837.h>
#include <ArduinoJson.h> // Include the ArduinoJson library

const char* ssid = "BDCHSPheonix";  // Define your Access Point SSID here
const char* password = "7861105253";  // Define your Access Point password here

AsyncWebServer server(80);

RTC_DS1307 RTC;
MS5837 sensor;

// Variables to store depth data
const int maxDataPoints = 100; // Adjust as needed
float depthData[maxDataPoints];
unsigned long timestampData[maxDataPoints];
int dataCount = 0;

// Declare WebSocket server
AsyncWebSocket websocketServer("/ws");

// Declare the function before using it
void graphDepthData();

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
  
  // Serve the HTML and JavaScript for the depth graph
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // ...
    request->send_P(200, "text/html", R"(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Depth Graph</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
      </head>
      <body>
        <canvas id="depthChart" width="400" height="200"></canvas>
        <script>
          var ctx = document.getElementById('depthChart').getContext('2d');
          var depthData = {
            labels: [],
            datasets: [{
              label: 'Depth (m)',
              borderColor: 'blue',
              backgroundColor: 'rgba(0, 0, 255, 0.2)',
              data: [],
            }]
          };
          var depthChart = new Chart(ctx, {
            type: 'line',
            data: depthData,
            options: {
              scales: {
                x: {
                  type: 'time',
                  time: {
                    unit: 'second',
                  },
                  title: {
                    display: true,
                    text: 'Time',
                  },
                },
                y: {
                  beginAtZero: true,
                  title: {
                    display: true,
                    text: 'Depth (m)',
                  },
                },
              },
            },
          });

          function addDataToChart(timestamp, depth) {
            depthData.labels.push(timestamp);
            depthData.datasets[0].data.push(depth);
            depthChart.update();
          }

          const websocketClient = new WebSocket('ws://' + location.host + '/ws');
          websocketClient.onmessage = function(event) {
            var data = JSON.parse(event.data);
            addDataToChart(data.timestamp, data.depth);
          };
        </script>
      </body>
      </html>
    )");
  });

  server.addHandler(&websocketServer);
  websocketServer.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
    if(type == WS_EVT_CONNECT){
      // ...
    } else if(type == WS_EVT_DATA){
      // Parse depth data and timestamp
      float depth = sensor.depth();
      unsigned long timestamp = millis();

      // Create a JSON object with depth and timestamp
      DynamicJsonDocument doc(64);
      doc["depth"] = depth;
      doc["timestamp"] = timestamp;
      String json;
      serializeJson(doc, json);

      // Send JSON data to the client
      client->text(json);

      // Store data points for later use
      if (dataCount < maxDataPoints) {
        depthData[dataCount] = depth;
        timestampData[dataCount] = timestamp;
        dataCount++;
      }
    }
  });

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

  // Check for a command over the serial interface
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'G' || command == 'g') {
      // Execute the graphing command
      graphDepthData();
    }
  }

  delay(1000);
}

// Function to graph the depth data
void graphDepthData() {
  // Create a JSON object with stored depth and timestamp data
  DynamicJsonDocument doc(1024); // Adjust the size as needed
  JsonArray depthArray = doc.createNestedArray("depth");
  JsonArray timestampArray = doc.createNestedArray("timestamp");

  for (int i = 0; i < dataCount; i++) {
    depthArray.add(depthData[i]);
    timestampArray.add(timestampData[i]);
  }

  String json;
  serializeJson(doc, json);

  // Send the JSON data to the web interface via WebSerial
  WebSerial.print(json);
  WebSerial.println();
  WebSerial.println("Graph data sent.");

  // You can add further processing or visualization here
  // For example, you might send the data to a cloud service or display it on a local screen.
  // This depends on your specific requirements.
}
