#include "Arduino.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <WebSocketsServer.h>

#include "config.hpp"
#include "camera.hpp"

WebSocketsServer webSocket = WebSocketsServer(81);
WiFiServer server(80);
uint8_t cam_num;
bool connected = false;

String index_html;

void liveCam(uint8_t num){
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("Frame buffer could not be acquired");
      return;
  }
  //replace this with your own function
  webSocket.sendBIN(num, fb->buf, fb->len);

  //return the frame buffer back to be reused
  esp_camera_fb_return(fb);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            cam_num = num;
            connected = true;
            break;
        case WStype_TEXT:
        case WStype_BIN:
        case WStype_ERROR:      
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void setup() {
  Serial.begin(115200);

  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  Config config;
  const String ssid = config.getSSID();
  const String password = config.getPassword();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
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

  Serial.println("");
  String IP = WiFi.localIP().toString();
  Serial.println("IP address: " + IP);
  digitalWrite(4, HIGH);
  index_html.replace("server_ip", IP);
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  const framesize_t frameSize = config.getFrameSize();
  // const framesize_t frameSize = FRAMESIZE_240X240;
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
