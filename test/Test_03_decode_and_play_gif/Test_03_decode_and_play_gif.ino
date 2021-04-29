#include <FastLED.h>

#include "SPIFFS.h"
#include "GifDecoder.h"

#define DEBUG

#ifdef DEBUG
 #define DEBUG_BUTTON
// #define DEBUG_SCREEN_CLEAR_CALLBACK
// #define DEBUG_UPDATE_SCREEN_CALLBACK
 #define DEBUG_DRAW_PIXEL_CALLBACK
 //#define DEBUG_FILE_SEEK_CALLBACK
 //#define DEBUG_FILE_POSITION_CALLBACK
 //#define DEBUG_FILE_READ_CALLBACK
 //#define DEBUG_FILE_READ_BLOCK_CALLBACK
#endif



#define LED_PIN           15           // Output pin for LEDs [5]
#define COLOR_ORDER       GRB         // Color order of LED string [GRB]
#define CHIPSET           WS2812B     // LED string type [WS2182B]
#define BRIGHTNESS        50          // Overall brightness [50]
#define kMatrixWidth      17
#define kMatrixHeight     17
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)                                       // Total number of Leds
#define LAST_VISIBLE_LED  220         // Last LED that's visible [102]

CRGB leds[ NUM_LEDS ];
uint8_t brightness = BRIGHTNESS;

// Helper to map XY coordinates to irregular matrix
uint16_t XY (uint16_t x, uint16_t y) {
  // any out of bounds address maps to the first hidden pixel
  if ( (x >= kMatrixWidth) || (y >= kMatrixHeight) ) {
    return (LAST_VISIBLE_LED + 1);
  }

  const uint16_t XYTable[] = {
   277, 269, 263, 259, 257, 255, 136, 119, 102,  85,  68, 253, 251, 247, 241, 233, 221,
   278, 270, 264, 260, 168, 153, 137, 120, 103,  86,  69,  53,  38, 248, 242, 234, 222,
   279, 271, 265, 183, 169, 154, 138, 121, 104,  87,  70,  54,  39,  25, 243, 235, 223,
   280, 272, 196, 184, 170, 155, 139, 122, 105,  88,  71,  55,  40,  26,  14, 236, 224,
   281, 207, 197, 185, 171, 156, 140, 123, 106,  89,  72,  56,  41,  27,  15,   5, 225,
   282, 208, 198, 186, 172, 157, 141, 124, 107,  90,  73,  57,  42,  28,  16,   6, 226,
   216, 209, 199, 187, 173, 158, 142, 125, 108,  91,  74,  58,  43,  29,  17,   7,   0,
   217, 210, 200, 188, 174, 159, 143, 126, 109,  92,  75,  59,  44,  30,  18,   8,   1,
   218, 211, 201, 189, 175, 160, 144, 127, 110,  93,  76,  60,  45,  31,  19,   9,   2,
   219, 212, 202, 190, 176, 161, 145, 128, 111,  94,  77,  61,  46,  32,  20,  10,   3,
   220, 213, 203, 191, 177, 162, 146, 129, 112,  95,  78,  62,  47,  33,  21,  11,   4,
   283, 214, 204, 192, 178, 163, 147, 130, 113,  96,  79,  63,  48,  34,  22,  12, 227,
   284, 215, 205, 193, 179, 164, 148, 131, 114,  97,  80,  64,  49,  35,  23,  13, 228,
   285, 273, 206, 194, 180, 165, 149, 132, 115,  98,  81,  65,  50,  36,  24, 237, 229,
   286, 274, 266, 195, 181, 166, 150, 133, 116,  99,  82,  66,  51,  37, 244, 238, 230,
   287, 275, 267, 261, 182, 167, 151, 134, 117, 100,  83,  67,  52, 249, 245, 239, 231,
   288, 276, 268, 262, 258, 256, 152, 135, 118, 101,  84, 254, 252, 250, 246, 240, 232
  };

  uint16_t i = (y * kMatrixWidth) + x;
  uint16_t j = XYTable[i];
  return j;
}




File file;
GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

void screenClearCallback() {
  #ifdef DEBUG_SCREEN_CLEAR_CALLBACK
  Serial.println(">>> screenClearCallback");
  #endif
  FastLED.clear();
}

void updateScreenCallback(){
  #ifdef DEBUG_UPDATE_SCREEN_CALLBACK
  Serial.println(">>> updateScreenCallback");
  #endif
  FastLED.show();
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue){
  #ifdef DEBUG_DRAW_PIXEL_CALLBACK
  Serial.printf(">>> drawPixelCallback, pos(%i, %i), color(%i, %i, %i)\n", x, y, red, green, blue);
  #endif

  leds[XY(x,y)] = CRGB(red, green, blue);
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

  FastLED.addLeds < CHIPSET, LED_PIN, COLOR_ORDER > (leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);


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

    Serial.println("end setup()...");

}

void loop() {
  decoder.decodeFrame();
}
