#include "SPIFFS.h"
#include "GifDecoder.h"

#define DEBUG

#ifdef DEBUG
 #define DEBUG_BUTTON
// #define DEBUG_SCREEN_CLEAR_CALLBACK
// #define DEBUG_UPDATE_SCREEN_CALLBACK
 //#define DEBUG_DRAW_PIXEL_CALLBACK
 //#define DEBUG_FILE_SEEK_CALLBACK
 //#define DEBUG_FILE_POSITION_CALLBACK
 //#define DEBUG_FILE_READ_CALLBACK
 //#define DEBUG_FILE_READ_BLOCK_CALLBACK
#endif

#define kMatrixWidth  17
#define kMatrixHeight 17

File file;
GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

void screenClearCallback() {
  #ifdef DEBUG_SCREEN_CLEAR_CALLBACK
  Serial.println(">>> screenClearCallback");
  #endif
  //matrix->clear();
  //FastLED.clear();
}

void updateScreenCallback(){
  #ifdef DEBUG_UPDATE_SCREEN_CALLBACK
  Serial.println(">>> updateScreenCallback");
  #endif
  //matrix->show();
  //FastLED.show();
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue){
  #ifdef DEBUG_DRAW_PIXEL_CALLBACK
  Serial.print(">>> drawPixelCallback ");
  if(x>8 || y>8){
    Serial.print(">>> pixel (");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.println(")");
  }
  #endif

  //matrix->drawPixel(x, y, FastLED_NeoMatrix::Color(red, green, blue));
  //leds[XY(x,y)] = CRGB(red, green, blue);
}

bool fileSeekCallback(unsigned long position){
  #ifdef DEBUG_FILE_SEEK_CALLBACK
  Serial.print(">>> fileSeekCallback  ");
  Serial.print("position: ");
  Serial.print (position);
  #endif
  bool r = file.seek(position);
  #ifdef DEBUG_FILE_SEEK_CALLBACK
  Serial.print(", r ");
  Serial.println(r);
  #endif
  return r;
}

unsigned long filePositionCallback(){
  #ifdef DEBUG_FILE_POSITION_CALLBACK
  Serial.println(">>> filePositionCallback  ");
  #endif
  return file.position();
}

int fileReadCallback(){
  #ifdef DEBUG_FILE_READ_CALLBACK
  Serial.println(">>> fileReadCallback");
  #endif
  return file.read();
}

int fileReadBlockCallback(void * buffer, int numberOfBytes){
  #ifdef DEBUG_FILE_READ_BLOCK_CALLBACK
  Serial.print(">>> fileReadBlockCallback  ");
  Serial.print("numberOfBytes: ");
  Serial.println(numberOfBytes);
  #endif

  int num_read = file.read((uint8_t *)buffer, numberOfBytes);

  #ifdef DEBUG_FILE_READ_BLOCK_CALLBACK
  Serial.print(", read ");
  Serial.print(num_read);
  Serial.print("  : ");
  Serial.println((char *)buffer);
  #endif

  return num_read;
}

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


    // setup gif decoder callbacks and start decoding
    decoder.setScreenClearCallback(screenClearCallback);
    decoder.setUpdateScreenCallback(updateScreenCallback);
    decoder.setDrawPixelCallback(drawPixelCallback);
    decoder.setFileSeekCallback(fileSeekCallback);
    decoder.setFilePositionCallback(filePositionCallback);
    decoder.setFileReadCallback(fileReadCallback);
    decoder.setFileReadBlockCallback(fileReadBlockCallback);
    decoder.startDecoding();

    Serial.println("end setup()");

}

void loop() {
  decoder.decodeFrame();
}
