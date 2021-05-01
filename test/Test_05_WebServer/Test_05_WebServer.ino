#include "PandaWebServer.h"

PandaWebServer server;

void setup() {
  Serial.begin(57600);
  Serial.println("start setup()...");

  server.setup();
}

void loop() {
  server.update();
}


