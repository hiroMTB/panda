
#include <FastLED.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include "SPIFFS.h"

#include "GifDecoder.h"

#define DEBUG

#ifdef DEBUG
 #define DEBUG_BUTTON
 #define DEBUG_SCREEN_CLEAR_CALLBACK
 #define DEBUG_UPDATE_SCREEN_CALLBACK
 #define DEBUG_DRAW_PIXEL_CALLBACK
 #define DEBUG_FILE_SEEK_CALLBACK
 #define DEBUG_FILE_POSITION_CALLBACK
 #define DEBUG_FILE_READ_CALLBACK
 #define DEBUG_FILE_READ_BLOCK_CALLBACK
#endif

#define PIN_SDCARD 10



#define LED_PIN           15           // Output pin for LEDs [5]
#define MODE_PIN          3           // Input pin for button to change mode [3]
#define PLUS_PIN          2           // Input pin for plus button [2]
#define MINUS_PIN         4           // Input pin for minus button [4]
#define MIC_PIN           A0          // Input pin for microphone [A6]
#define COLOR_ORDER       GRB         // Color order of LED string [GRB]
#define CHIPSET           WS2812B     // LED string type [WS2182B]
#define BRIGHTNESS        50          // Overall brightness [50]
#define LAST_VISIBLE_LED  220         // Last LED that's visible [102]
#define MAX_MILLIAMPS     5000        // Max current in mA to draw from supply [500]
#define SAMPLE_WINDOW     100         // How many ms to sample audio for [100]
#define DEBOUNCE_MS       20          // Number of ms to debounce the button [20]
#define LONG_PRESS        500         // Number of ms to hold the button to count as long press [500]
#define PATTERN_TIME      5         // Seconds to show each pattern on autoChange [10]
#define kMatrixWidth      17          // Matrix width [15]
#define kMatrixHeight     17          // Matrix height [11]
#define NUM_LEDS (kMatrixWidth * kMatrixHeight)                                       // Total number of Leds
#define MAX_DIMENSION ((kMatrixWidth>kMatrixHeight) ? kMatrixWidth : kMatrixHeight)   // Largest dimension of matrix

CRGB leds[ NUM_LEDS ];
uint8_t brightness = BRIGHTNESS;
uint8_t soundSensitivity = 10;

// Used to check RAM availability. Usage: Serial.println(freeRam());
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Gif
File file;
GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

// Button stuff
uint8_t buttonPushCounter = 10;
uint8_t state = 0;
bool autoChangeVisuals = true;
Button modeBtn(MODE_PIN, DEBOUNCE_MS);
Button plusBtn(PLUS_PIN, DEBOUNCE_MS);
Button minusBtn(MINUS_PIN, DEBOUNCE_MS);

void incrementButtonPushCounter() {
  buttonPushCounter = (buttonPushCounter + 1) %11;
  EEPROM.write(1, buttonPushCounter);
}

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

void screenClearCallback() {
  #ifdef DEBUG_SCREEN_CLEAR_CALLBACK
  Serial.println(">>> screenClearCallback");
  #endif
  //matrix->clear();
  FastLED.clear();
}

void updateScreenCallback(){
  #ifdef DEBUG_UPDATE_SCREEN_CALLBACK
  Serial.println(">>> updateScreenCallback");
  #endif
  //matrix->show();
  FastLED.show();
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue){
  #ifdef DEBUG_DRAW_PIXEL_CALLBACK
  Serial.println(">>> drawPixelCallback");
  if(x>8 || y>8){
    Serial.print(">>> pixel (");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.println(")");
  }
  #endif

  //matrix->drawPixel(x, y, FastLED_NeoMatrix::Color(red, green, blue));
  leds[XY(x,y)] = CRGB(red, green, blue);
}

bool fileSeekCallback(unsigned long position){
  #ifdef DEBUG_FILE_SEEK_CALLBACK
  Serial.println(">>> fileSeekCallback");
  Serial.print(">>> position: ");
  Serial.println(position);
  #endif
  bool r = file.seek(position);
  #ifdef DEBUG_FILE_SEEK_CALLBACK
  Serial.println(">>> r");
  Serial.println(r);
  #endif
  return r;
}

unsigned long filePositionCallback(){
  #ifdef DEBUG_FILE_POSITION_CALLBACK
  Serial.println(">>> filePositionCallback");
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
  Serial.println(">>> fileReadBlockCallback");
  Serial.println(numberOfBytes);
  #endif

  int num_read = file.read((uint8_t *)buffer, numberOfBytes);

  #ifdef DEBUG_FILE_READ_BLOCK_CALLBACK
  Serial.print("read ");
  Serial.println(num_read);
  Serial.print(": ");
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

  modeBtn.begin();
  plusBtn.begin();
  minusBtn.begin();
  buttonPushCounter = (int)EEPROM.read(1);    // load previous setting
  
  Serial.print(F("Starting pattern "));
  Serial.println(buttonPushCounter);


  // open Gif file
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  file = SPIFFS.open("/test.gif");

  if (!file) {
    #ifdef DEBUG
    Serial.println("file open failed");
    #endif
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
}

void checkBrightnessButton() {
  // On all none-sound reactive patterns, plus and minus control the brightness
  plusBtn.read();
  minusBtn.read();

  if (plusBtn.wasReleased()) {
    brightness += 20;
    FastLED.setBrightness(brightness);
  }

  if (minusBtn.wasReleased()) {
    brightness -= 20;
    FastLED.setBrightness(brightness);
  }
}

void checkSoundLevelButton(){
  // On all sound reactive patterns, plus and minus control the sound sensitivity
  plusBtn.read();
  minusBtn.read();

  if (plusBtn.wasReleased()) {
    // Increase sound sensitivity
    soundSensitivity -= 1;
    if (soundSensitivity == 0) soundSensitivity = 1;
  }

  if (minusBtn.wasReleased()) {
    // Decrease sound sensitivity
    soundSensitivity += 1;
  }
}

bool checkModeButton() {  
  modeBtn.read();

  switch (state) {
    case 0:                
      if (modeBtn.wasReleased()) {
        incrementButtonPushCounter();
        Serial.print(F("Short press, pattern "));
        Serial.println(buttonPushCounter);
        autoChangeVisuals = false;
        return true;
      }
      else if (modeBtn.pressedFor(LONG_PRESS)) {
        state = 1;
        return true;
      }
      break;

    case 1:
      if (modeBtn.wasReleased()) {
        state = 0;
        Serial.print(F("Long press, auto, pattern "));
        Serial.println(buttonPushCounter);
        autoChangeVisuals = true;
        return true;
      }
      break;
  }

  if(autoChangeVisuals){
    EVERY_N_SECONDS(PATTERN_TIME) {
      incrementButtonPushCounter();
      Serial.print("Auto, pattern ");
      Serial.println(buttonPushCounter); 
      return true;
    }
  }  

  return false;
}


// Run selected pattern
void loop() {
    decoder.decodeFrame();
}
