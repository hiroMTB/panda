#include "PandaWebServer.h"

PandaWebServer server;

void gifPlayCallback(String filename){
  // notify file change to gifPlayer
  Serial.printf("Gif Play Callback, file: %s\n", filename);
}

void setup() {
  Serial.begin(57600);
  Serial.println("start setup()...");  
  server.setGifPlayCallback(gifPlayCallback);
  server.setup();
}

void loop() {
  server.update();
}


