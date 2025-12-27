#include <Arduino.h>
#include "WiFiFSManager.h"

WiFiFSManager mgr(80, true);

void setup() {
  Serial.begin(115200); delay(500);
  mgr.setFileAuth("files", "files123");
  mgr.begin("ESP32_AP", "12345678", "esp32fs");
}

void loop() { mgr.handle(); }
