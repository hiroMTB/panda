#include "GifPlayer.h"
#include "PandaWebServer.h"

GifPlayer gifPlayer;
PandaWebServer server;

// Callback When server receive play request e.g. /play?test.gif
// then we change gifPlayer's currentFilename
void gifPlayCallback(String filename){
  Serial.printf("Change Gif, file: %s\n", filename);
  gifPlayer.setCurrentFilename(filename);
}

void setup() {
  Serial.begin(57600);
  Serial.println("start setup()...");

  gifPlayer.setup();

  server.setup();
  server.setGifPlayCallback(gifPlayCallback);

  Serial.println("end setup()...");
}

void loop() {
  server.update();
  gifPlayer.update();
}


