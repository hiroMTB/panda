#include "GifPlayer.h"

GifPlayer gifPlayer;

void setup() {
  Serial.begin(57600);
  Serial.println("start setup()...");

  gifPlayer.setup();
}

void loop() {
  gifPlayer.update();
}
