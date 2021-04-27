/* Helper functions for an two-dimensional XY matrix of pixels.

This special 'XY' code lets you program the RGB Shades
if it's was just a plain 16x5 matrix.  

Writing to and reading from the 'holes' in the layout is 
also allowed; holes retain their data, it's just not displayed.

You can also test to see if you're on- or off- the layout
like this
  if( XY(x,y) > LAST_VISIBLE_LED ) { ...off the layout...}

X and Y bounds checking is also included, so it is safe
to just do this without checking x or y in your code:
  leds[ XY(x,y) ] == CRGB::Red;
All out of bounds coordinates map to the first hidden pixel.

 https://macetech.github.io/FastLED-XY-Map-Generator/
      0   1   2   3   4   5   6   7   8   9  10  11   12 13  14
   +-----------------------------------------------------------
 0 | 14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
 1 | 15  16  17  18  19  20  21  22  23  24  25  26  27  28  29
 2 | 44  43  42  41  40  39  38  37  36  35  34  33  32  31  30
 3 | 45  46                      47                      48  49
 4 | 58  57  56              55  54  53              52  51  50
 5 | 59  60  61  62          63  64  65          66  67  68  69
 6 |     80  79  78      77  76  75  74  73      72  71  70  
 7 |                     81  82  83  84  85          
 8 |                 92  91  90  89  88  87  86        
 9 |                     93  94  95  96  97          
10 |                     102 101 100 99  98    
*/

#include <FastLED.h>
#include <EEPROM.h>
#include <JC_Button.h>

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

// Include various patterns
#include "Sound.h"
#include "Rainbow.h"
#include "Fire.h"
#include "Squares.h"
#include "Circles.h"
#include "Plasma.h"
#include "Matrix.h"
#include "CrossHatch.h"
#include "Drops.h"
#include "Noise.h"
#include "Snake.h"

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

void setup() {
  FastLED.addLeds < CHIPSET, LED_PIN, COLOR_ORDER > (leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  modeBtn.begin();
  plusBtn.begin();
  minusBtn.begin();
  buttonPushCounter = (int)EEPROM.read(1);    // load previous setting
  
  Serial.begin(57600);
  Serial.print(F("Starting pattern "));
  Serial.println(buttonPushCounter);
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

// Functions to run patterns. Done this way so each class stays in scope only while
// it is active, freeing up RAM once it is changed.

void runSound(){
  bool isRunning = true;
  Sound sound = Sound();
  while(isRunning) isRunning = sound.runPattern();
}

void runRainbow(){
  bool isRunning = true;
  Rainbow rainbow = Rainbow();
  while(isRunning) isRunning = rainbow.runPattern();
}

void runFire(){
  bool isRunning = true;
  Fire fire = Fire();
  while(isRunning) isRunning = fire.runPattern();
}

void runSquares(){
  bool isRunning = true;
  Squares squares = Squares();
  while(isRunning) isRunning = squares.runPattern();
}

void runCircles(){
  bool isRunning = true;
  Circles circles = Circles();
  while(isRunning) isRunning = circles.runPattern();
}

void runPlasma(){
  bool isRunning = true;
  Plasma plasma = Plasma();
  while(isRunning) isRunning = plasma.runPattern();
}

void runMatrix(){
  bool isRunning = true;
  Matrix matrix = Matrix();
  while(isRunning) isRunning = matrix.runPattern();
}

void runCrossHatch(){
  bool isRunning = true;
  CrossHatch crossHatch = CrossHatch();
  while(isRunning) isRunning = crossHatch.runPattern();
}

void runDrops(){
  bool isRunning = true;
  Drops drops = Drops();
  while(isRunning) isRunning = drops.runPattern();
}

void runNoise(){
  bool isRunning = true;
  Noise noise = Noise();
  while(isRunning) {
    isRunning = noise.runPattern();
  }
}

void runSnake(){
  bool isRunning = true;
  Snake snake = Snake();
  while(isRunning) {
    isRunning = snake.runPattern();
  }
}

// Run selected pattern
void loop() {
  switch (buttonPushCounter) {
    case 0:
     runNoise();
  //   runSound();
      break;
    case 1:
      runRainbow();
      break;
    case 2:
      runFire();
      break;
    case 3:
      runSquares();
      break;
    case 4:
      runCircles();
      break;
    case 5:
      runPlasma();
      break;
    case 6:
      runMatrix();
      break;
    case 7:
      runCrossHatch();
      break;
    case 8:
      runDrops();
      break;
    case 9:
      runSquares();
      break;
    case 10:
      runSnake();
      break;
  }
}
