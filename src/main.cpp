#include "Arduino.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>

#include "config.hpp"
#include "camera.hpp"

// Motor pins
constexpr uint8_t motor1 = 12;
constexpr uint8_t motor2 = 13;

WebSocketsServer webSocket = WebSocketsServer(81);
WiFiServer server(80);
uint8_t cam_num;
bool connected = false;

String index_html;

void liveCam(uint8_t num){
  // Capture a frame
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Frame buffer could not be acquired");
      return;
  }

  // Send the frame buffer to the client
  webSocket.sendBIN(num, fb->buf, fb->len);

  // Return the frame buffer back to be reused
  esp_camera_fb_return(fb);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            cam_num = num;
            connected = true;
            break;
        case WStype_TEXT:
            if (strcmp((char*)payload, "motor1on") == 0) {
                digitalWrite(motor1, HIGH);
            } else if (strcmp((char*)payload, "motor1off") == 0) {
                digitalWrite(motor1, LOW);
            } else if (strcmp((char*)payload, "motor2on") == 0) {
                digitalWrite(motor2, HIGH);
            } else if (strcmp((char*)payload, "motor2off") == 0) {
                digitalWrite(motor2, LOW);
            }
            break;
        case WStype_BIN:
        case WStype_ERROR:      
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void loadWebPage() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  while(file.available()){
    index_html += char(file.read());
  }
  file.close();
}

void setup() {
  // Setup serial monitor
  Serial.begin(115200);

  // Motor pins
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);

  // Flash LED
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  Config config;
  const String ssid = config.getSSID();
  const String password = config.getPassword();

  // Serial.println("Connecting to WiFi");
  // Serial.println("SSID: " + ssid);
  // Serial.println("Password: " + password);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
  }

  loadWebPage();

  // Print the IP address on the serial monitor
  Serial.println("");
  String IP = WiFi.localIP().toString();
  Serial.println("IP address: " + IP);

  // Turn on flash LED
  // digitalWrite(4, HIGH);

  index_html.replace("server_ip", IP);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // const framesize_t frameSize = config.getFrameSize();
  // const framesize_t frameSize = FRAMESIZE_240X240;
  const framesize_t frameSize = FRAMESIZE_96X96;
  Serial.println("Frame size: " + String(frameSize));
  configCamera(frameSize);
}

void http_resp(){
  WiFiClient client = server.available();
  if (client.connected() && client.available()) {                   
    client.flush();          
    client.print(index_html);
    client.stop();
  }
}

void loop() {
  http_resp();
  webSocket.loop();
  if(connected == true){
    liveCam(cam_num);
  }
}
