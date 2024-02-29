#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Arduino.h"
#include <esp_camera.h>

class Config {
    public:
        Config() {
            // Parse using SPIFFS
            File config = SPIFFS.open("/config.ini", "r");
            if (!config) {
                Serial.println("Failed to open config file for reading");
                return;
            }
            String content;
            while(config.available()){
                content += char(config.read());
            }
            config.close();

            ssid = content.substring(content.indexOf("ssid =") + 7, content.indexOf("\n", content.indexOf("ssid ="))).c_str();
            password = content.substring(content.indexOf("password =") + 11, content.indexOf("\n", content.indexOf("password ="))).c_str();

            // 0 96x96, 1 320x240, 2 640x480, 3 800x600, 4 1024x768, 5 1280x1024, 6 1600x1200
            int frameSizeInt = content.substring(content.indexOf("frameSize =") + 12, content.indexOf("\n", content.indexOf("frameSize ="))).toInt();
            Serial.println("Frame size: " + String(frameSizeInt));
            switch (frameSizeInt) {
                case 0:
                    frameSize = FRAMESIZE_QVGA;
                    break;
                case 1:
                    frameSize = FRAMESIZE_QVGA;
                    break;
                case 2:
                    frameSize = FRAMESIZE_VGA;
                    break;
                case 3:
                    frameSize = FRAMESIZE_SVGA;
                    break;
                case 4:
                    frameSize = FRAMESIZE_XGA;
                    break;
                case 5:
                    frameSize = FRAMESIZE_SXGA;
                    break;
                case 6:
                    frameSize = FRAMESIZE_UXGA;
                    break;
                default:
                    frameSize = FRAMESIZE_240X240;
                    break;
            }
        }

        String getSSID() const {
            return ssid;
        }

        String getPassword() const {
            return password;
        }

        framesize_t getFrameSize() const {
            return frameSize;
        }

    private:
        String ssid;
        String password;
        framesize_t frameSize;
};

#endif // CONFIG_HPP
