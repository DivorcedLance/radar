// Connect to: http://192.168.1.78/

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <NTPClient.h>
#include "index.h"
#include <sstream>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char* ssid = "FAMILIA ENRIQUE";
const char* password = "10130528";

const uint8_t rx_pin = 16;
const uint8_t tx_pin = 17;

const int pinLed1 = 25;

int detectionThreshold = 18;

String received_str = "";

bool state = false;

int currentMillis = 0;
int lastDetectionMillis = 0;

int previousSendReportMillis = 0;
int sendReportIntervalMillis = 15000;
String formattedTime;

const int reportSize = 15;

struct Report
{
  int inputADC;
  int filteredInputADC;
  int state;
  int millis;
  String formattedTime;
};

struct Settings
{
  int detectionThreshold;
  int sendReportIntervalMillis;
};

int inputADC;
int filteredInputADC;

Report reports[reportSize];
String reportJSON = "";
Settings settings[1];
String settingsJSON = "";
String sendReportIntervalMillisStr = "";
String detectionThresholdStr = "";

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
        webSocket.broadcastTXT("Bienvenido");
        webSocket.broadcastTXT(settingsJSON);
        webSocket.broadcastTXT(reportJSON);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Received text: %s\n", num, payload);
      String msg = String((char*)payload);
      msg.trim();
      webSocket.broadcastTXT("Received:  " + msg);
      if (msg == "toggle") {
        toogleState();
      } else if (msg == "on") {
        encenderLED(pinLed1);
      } else if (msg == "off") {
        apagarLED(pinLed1);
      } else if (msg == "report") {
        updateReportJSON();
        webSocket.broadcastTXT("" + reportJSON);
      } else if (msg == "settings") {
        updateSettingsJSON();
        webSocket.broadcastTXT("" + settingsJSON);
      } else if (msg[msg.length() - 1] == 'i') {
        sendReportIntervalMillis = msg.substring(0, msg.length() - 1).toInt();
        updateSettingsJSON();
        webSocket.broadcastTXT(settingsJSON);
      } else if (msg[msg.length() - 1] == 't') {
        detectionThreshold = msg.substring(0, msg.length() - 1).toInt();
        updateSettingsJSON();
        webSocket.broadcastTXT(settingsJSON);
      }
      break;
    }
  }

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, rx_pin, tx_pin);
  delay(1000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  pinMode(pinLed1, OUTPUT);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("Web Server: received a web page request");
    String html = HTML_CONTENT;
    request->send(200, "text/html", html);
  });

  server.on("/inputADC", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("ESP32 Web Server: New request received:");  
    Serial.println("GET /data");                         
    float data = inputADC;
    String dataStr = String(data, 2);
    request->send(200, "text/plain", dataStr);
  });

  server.on("/filteredInputADC", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("ESP32 Web Server: New request received:");  
    Serial.println("GET /data");                         
    float data = filteredInputADC;
    String dataStr = String(data, 2);
    request->send(200, "text/plain", dataStr);
  });

  server.begin();
  Serial.print("ESP32 Web Server's IP address: ");
  Serial.println(WiFi.localIP());
  
  encenderLED(pinLed1);
  delay(500);
  apagarLED(pinLed1);
  
  timeClient.begin();
  timeClient.setTimeOffset(-18000); // UTC -5 for Colombia
}

void loop() {
  webSocket.loop();
  readUART();
  sendReport();
}

void readUART() {
    if (Serial1.available()) {
        while (Serial1.available()) {
            char received_char = Serial1.read();
            if (received_char == '\n') {
                continue;
            }
            if (received_char == '.') {
                received_str += received_char;
                break;
            }
            received_str += received_char;
        }
        if (received_str.length() > 0 && received_str[received_str.length() - 1] == '.') {
            received_str.remove(received_str.length() - 1);
            int dash_pos = received_str.indexOf('-');

            if (dash_pos != -1) {
                String inputADC_str = received_str.substring(0, dash_pos);
                String filteredInputADC_str = received_str.substring(dash_pos + 1);

                inputADC = inputADC_str.toInt();
                filteredInputADC = filteredInputADC_str.toInt();
                updateReport();

                if (filteredInputADC > detectionThreshold) {
                    currentMillis = millis();
                    //Serial.println("Detected");
                    if (currentMillis - lastDetectionMillis > 2000) {
                        lastDetectionMillis = currentMillis;
                        Serial.println("Movimiento detectado");
                        Serial.println(filteredInputADC_str);
                        toogleState();
                        updateReportJSON();
                        webSocket.broadcastTXT("Report :" + reportJSON);
                    }
                }
            }

            received_str = "";
        }
    }
}

void sendReport() {
    if (millis() - previousSendReportMillis > sendReportIntervalMillis) {
        previousSendReportMillis = millis();
        updateReportJSON();
        webSocket.broadcastTXT("" + reportJSON);
        Serial.println(""  + reportJSON);
    }
}

void updateReportJSON() {
    String newReportJSON = "[";
    
    for (int i = 0; i < reportSize; i++) {
        newReportJSON += "\"" + String(reports[i].millis) + "\": {";
        newReportJSON += "\"inputADC\": " + String(reports[i].inputADC) + ",";
        newReportJSON += "\"filteredInputADC\": " + String(reports[i].filteredInputADC) + ",";
        newReportJSON += "\"formattedTime\": \"" + String(reports[i].formattedTime) + "\",";
        newReportJSON += "\"state\": " + String(reports[i].state);
        newReportJSON += "}";
        if (i < reportSize - 1) {
            newReportJSON += ",";
        }
    }

    newReportJSON += "]";
    reportJSON = newReportJSON;
}

void updateSettingsJSON() {
    updateSettings();
    String newSettingsJSON = "{";
    
    newSettingsJSON += "\"settings\": {";
    newSettingsJSON += "\"detectionThreshold\": " + String(settings[0].detectionThreshold ) + ",";
    newSettingsJSON += "\"sendReportIntervalMillis\": " + String(settings[0].sendReportIntervalMillis);
    newSettingsJSON += "}";

    newSettingsJSON += "}";
    settingsJSON = newSettingsJSON;
}


void updateReport() {
    formattedTime = getFormattedTime();
    
    for (int i = reportSize - 1; i > 0; i--) {
        reports[i] = reports[i - 1];
    }

    reports[0].inputADC = inputADC;
    reports[0].filteredInputADC = filteredInputADC;
    reports[0].state = state;
    reports[0].millis = millis();
    reports[0].formattedTime = formattedTime;
}

void updateSettings() {
  settings[0].detectionThreshold = detectionThreshold;
  settings[0].sendReportIntervalMillis = sendReportIntervalMillis;
}

String getFormattedTime() {
    timeClient.update();
    return timeClient.getFormattedTime();
}

void toogleState() {
    state = !state;
    if (state) {
        Serial.println("1");
        encenderLED(pinLed1);
    }
    else {
        Serial.println("0");
        apagarLED(pinLed1);
    }
    delay(500);
}

void encenderLED(int pinLed) {
    digitalWrite(pinLed, HIGH);
}

void apagarLED(int pinLed) {
    digitalWrite(pinLed, LOW);
}