#include "SPIFFS.h"

File file;

void setup() {
    Serial.begin(57600);
    Serial.println("start setup()...");

    if(!SPIFFS.begin(true)){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
  
    // open Gif file
    file = SPIFFS.open("/test.gif");
    //file = SPIFFS.open("/test.txt");

    if (!file) {
      Serial.println("file open failed");
      return;
    }else{
      Serial.println("file open success!!");    
    }
}

void loop() {
  
}
