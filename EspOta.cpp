#include "EspOta.h"
#include "Arduino.h"
#include "Preferences.h"
#include "Update.h"
#include "WiFi.h"

WiFiClient espOtaWiFiClient;
Preferences espOtaPreferences;

// Constructor variables
String _host;
int _port;
String _bin;
const char* _pKey;
String updateTime = "";

// ETag variables
String currentEtag;
String newEtag = "";
const char* etagId = "etag";                 // Key name is limited to 15 chars.
const char* etagUpdateTime = "etag_update";  // Key name is limited to 15 chars.

// Validation variables
const int maxValidationAttempts = 3;
int validationAttemptIndex = 1;
int contentLength = 0;
bool isValidContentType = false;
bool sameETag = false;

// Utility to extract header value from headers
String getHeaderValue(String header, String headerName) { return header.substring(strlen(headerName.c_str())); }

EspOta::EspOta(const String host, const int port, const String bin, const char* pKey) {
    _host = host;
    _port = port;
    _bin = bin;
    _pKey = pKey;
}
EspOta::~EspOta() {}

void EspOta::updateEntries(const String host, const int port, const String bin) {
    _host = host;
    _port = port;
    _bin = bin;
}

void EspOta::begin(String timestamp) {
    Serial.println("");
    Serial.println("Begin OTA update");

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Device not connected to WiFi");
        return;
    }

    currentEtag = "";
    sameETag = false;
    contentLength = 0;
    isValidContentType = false;
    validationAttemptIndex = 0;

    espOtaPreferences.begin(_pKey, false);
    this->getCurrentETag();

    while (!contentLength && !isValidContentType && !sameETag && validationAttemptIndex < maxValidationAttempts) {
        this->validate();
    }

    if (sameETag) {
        this->end();
        return;
    }

    this->update(timestamp);
}

void EspOta::getCurrentETag() {
    currentEtag = espOtaPreferences.getString(etagId, "");
    Serial.println("Current OTA ETag: " + currentEtag);
}

void EspOta::validate() {
    Serial.println("Validate attempt: " + String(validationAttemptIndex));
    Serial.println("Connecting to: " + String(_host));
    // Connect to S3
    if (espOtaWiFiClient.connect(_host.c_str(), _port)) {
        // Connection Succeed.
        // Fecthing the bin
        Serial.println("Fetching Bin: " + String(_bin));

        // Get the contents of the bin file
        espOtaWiFiClient.print(String("GET ") + _bin + " HTTP/1.1\r\n" + "Host: " + _host + "\r\n" + "Cache-Control: no-cache\r\n" +
                               "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (espOtaWiFiClient.available() == 0) {
            if (millis() - timeout > 5000) {
                Serial.println("Client Timeout!");
                break;
                espOtaWiFiClient.stop();
                validationAttemptIndex += 1;
            }
        }

        /*
           Response Structure
            HTTP/1.1 200 OK
            x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
            x-amz-request-id: 2D56B47560B764EC
            Date: Wed, 14 Jun 2017 03:33:59 GMT
            Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
            ETag: "d2afebbaaebc38cd669ce36727152af9"
            Accept-Ranges: bytes
            Content-Type: application/octet-stream
            Content-Length: 357280
            Server: AmazonS3

            {{BIN FILE CONTENTS}}

        */

        while (espOtaWiFiClient.available()) {
            // read line till /n
            String line = espOtaWiFiClient.readStringUntil('\n');
            // remove space, to check if the line is end of headers
            line.trim();

            // if the the line is empty,
            // this is end of headers
            // break the while and feed the
            // remaining `client` to the
            // Update.writeStream();
            if (!line.length()) {
                // headers ended
                break;  // and get the OTA started
            }

            // Check if the HTTP Response is 200
            // else break and Exit Update
            if (line.startsWith("HTTP/1.1")) {
                if (line.indexOf("200") < 0) {
                    Serial.println("Got a non 200 status code from server. Retry!");
                    espOtaWiFiClient.stop();
                    validationAttemptIndex += 1;
                }
            }

            // Check previous OTA update ETag
            if (line.startsWith("ETag: ")) {
                newEtag = getHeaderValue(line, "ETag: ");
                Serial.println("New ETag:" + newEtag);
                if (currentEtag == newEtag) {
                    Serial.println("ETag the same.");
                    espOtaWiFiClient.stop();
                    sameETag = true;
                }
            }

            // extract headers here
            // Start with content length
            if (line.startsWith("Content-Length: ")) {
                contentLength = atoi((getHeaderValue(line, "Content-Length: ")).c_str());
                Serial.println("Got " + String(contentLength) + " bytes from server");
            }

            // Next, the content type
            if (line.startsWith("Content-Type: ")) {
                String contentType = getHeaderValue(line, "Content-Type: ");
                Serial.println("Got " + contentType + " payload.");
                if (contentType == "application/octet-stream") {
                    isValidContentType = true;
                }
            }
        }
    } else {
        // Connect to S3 failed
        Serial.println("Connection to " + String(_host) + " failed. Retry!");
        espOtaWiFiClient.stop();
        validationAttemptIndex += 1;
    }
}

void EspOta::update(String timestamp) {
    // check contentLength and content type
    if (contentLength && isValidContentType) {
        // Check if there is enough to OTA Update
        bool canBegin = Update.begin(contentLength);

        // If yes, begin
        if (canBegin) {
            Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
            // No activity would appear on the Serial monitor
            // So be patient. This may take 2 - 5mins to complete
            size_t written = Update.writeStream(espOtaWiFiClient);

            if (written == contentLength) {
                Serial.println("Written : " + String(written) + " successfully");
            } else {
                Serial.println("Written only : " + String(written) + "/" + String(contentLength));
            }

            if (Update.end()) {
                Serial.println("OTA done!");
                if (Update.isFinished()) {
                    Serial.println("timestamp: " + timestamp);
                    espOtaPreferences.putString(etagUpdateTime, timestamp);

                    Serial.println("Save new ETag: " + newEtag);
                    espOtaPreferences.putString(etagId, newEtag);

                    Serial.println("Update successfully completed. Rebooting.");
                    ESP.restart();
                } else {
                    Serial.println("Update not finished? Something went wrong!");
                    this->end();
                }
            } else {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                this->end();
            }
        } else {
            // not enough space to begin OTA
            // Understand the partitions and
            // space availability
            Serial.println("Not enough space to begin OTA");
            this->end();
        }
    } else {
        Serial.println("There was no content in the response");
        this->end();
    }
}

void EspOta::end() {
    espOtaWiFiClient.stop();
    espOtaPreferences.end();
    Serial.println("Exiting OTA Update.");
}

String EspOta::getUpdateTime() {
    if (updateTime == "") {
        espOtaPreferences.begin(_pKey, true);
        updateTime = espOtaPreferences.getString(etagUpdateTime, "-");
        espOtaPreferences.end();
    }
    return updateTime;
}