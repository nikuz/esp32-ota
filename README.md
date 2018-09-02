# ESP32 OTA update arduino library

Example:
```c++
#include <WiFi.h>
#include <EspOta.h>

const char* SSID = "******";
const char* PSWD = "******";
const char* TAG = "my-app";

// http://s3.eu-central-1.amazonaws.com/update-bucket/update.bin
const String otaHost = "s3.eu-central-1.amazonaws.com";
const int    otaPort = 80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
const String otaBin = "update-bucket/update.bin";


EspOta otaUpdate(otaHost, otaPort, otaBin, TAG);

void setup() {
  //Begin Serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

  Serial.println("Connecting to " + String(SSID));

  // Connect to provided SSID and PSWD
  WiFi.begin(SSID, PSWD);

  // Wait for connection to establish
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); // Keep the serial monitor lit!
    delay(500);
  }

  // Execute OTA Update
  otaUpdate.begin();
}

void loop() {
  // chill
}
```

If server response contains the ETag header then library will compare it with stored one in Preferences.