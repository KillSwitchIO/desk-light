#include <FastLED.h>
#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

//ESP32
#define DATA_PIN    23
#define ARRAY_UP    19
#define ARRAY_DOWN  21

//Encoder
#define ROTARY_ENCODER_A_PIN 25
#define ROTARY_ENCODER_B_PIN 26
#define ROTARY_ENCODER_BUTTON_PIN 27
#define ROTARY_ENCODER_VCC_PIN -1

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN);

int test_limits = 2;

enum LedEncoderMode{
	LED_COLOR = 0,
	LED_MODE = 1,
	LED_BRIGHTNESS = 2,
};
LedEncoderMode rotaryMode = LED_COLOR;

LedEncoderMode nextEncoderMode(LedEncoderMode rotaryMode)
{
  LedEncoderMode temp = LED_COLOR;
   switch(rotaryMode)
   {
      case LED_COLOR:
        temp = LED_MODE;
        break;
      case LED_MODE:
        temp = LED_BRIGHTNESS;
        break;
      case LED_BRIGHTNESS:
        temp = LED_COLOR;
        break;
      default:
        temp = LED_COLOR;
        break;
   }
   return temp;
}

// ESP8266
// #define DATA_PIN    5
// #define ARRAY_UP    14
// #define ARRAY_DOWN  12
int lastButtonStateArrayDown = LOW;   // the previous reading from the input pin
int lastButtonStateArrayUp = LOW;   // the previous reading from the input pin
int buttonStateDown = LOW;   // the previous reading from the input pin
int buttonStateUp = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTimeArrayDown = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTimeArrayUp = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

//#define CLK_PIN   4
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    103
CRGB leds[NUM_LEDS];

#define MAXBRIGHTNESS      755
int     BRIGHTNESS =       MAXBRIGHTNESS;
#define FRAMES_PER_SECOND  120

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t solidHue = 340; // rotating "base color" used by many of the patterns

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}


void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void blue()
{
  fill_solid(leds, NUM_LEDS, CRGB::Blue);
}


void purple()
{
  fill_solid(leds, NUM_LEDS, CRGB::DarkViolet);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { purple, blue, rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

void nextPattern()
{
  Serial.write("Next Pattern");
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void previousPattern()
{
  Serial.write("Previous Pattern");
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber - 1) % ARRAY_SIZE( gPatterns);
}

void brightnessUp()
{
  BRIGHTNESS = (BRIGHTNESS + 1) % MAXBRIGHTNESS; 
  FastLED.setBrightness(BRIGHTNESS);
}

void brightnessDown()
{
  BRIGHTNESS = (BRIGHTNESS - 1) % MAXBRIGHTNESS; 
  FastLED.setBrightness(BRIGHTNESS);
}

void hueUp()
{
  solidHue = (solidHue + 10) % 360; 
  fill_solid(leds, NUM_LEDS, CHSV(solidHue, 255, BRIGHTNESS));
}

void hueDown()
{
  solidHue = (solidHue - 10) % 360;
  fill_solid(leds, NUM_LEDS, CHSV(solidHue, 255, BRIGHTNESS));
}

void debounceArrayDown()
{
  int pinReading = digitalRead(ARRAY_DOWN);
  if (pinReading != lastButtonStateArrayDown) {
      // reset the debouncing timer
      lastDebounceTimeArrayDown = millis();
    }

    if ((millis() - lastDebounceTimeArrayDown) > debounceDelay) {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:

      // if the button state has changed:
      if (pinReading != buttonStateDown) {
        buttonStateDown = pinReading;

        // only toggle the LED if the new button state is HIGH
        if (buttonStateDown == HIGH) {
          previousPattern();
        }
      }
    }

    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonStateArrayDown = pinReading;
}

void debounceArrayUp()
{
  int pinReading = digitalRead(ARRAY_UP);
  if (pinReading != lastButtonStateArrayUp) {
      // reset the debouncing timer
      lastDebounceTimeArrayUp = millis();
    }

    if ((millis() - lastDebounceTimeArrayUp) > debounceDelay) {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:

      // if the button state has changed:
      if (pinReading != buttonStateUp) {
        buttonStateUp = pinReading;

        // only toggle the LED if the new button state is HIGH
        if (buttonStateUp == HIGH) {
          nextPattern();
        }
      }
    }

    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonStateArrayUp = pinReading;
}

void rotary_onButtonClick_sample() {
	//rotaryEncoder.reset();
	//rotaryEncoder.disable();
	rotaryEncoder.setBoundaries(-test_limits, test_limits, false);
	test_limits *= 2;
}

void rotary_loop_sample() {
	//first lets handle rotary encoder button click
	if (rotaryEncoder.currentButtonState() == BUT_RELEASED) {
		//we can process it here or call separate function like:
		rotary_onButtonClick_sample();
	}

	//lets see if anything changed
	int16_t encoderDelta = rotaryEncoder.encoderChanged();
	
	//optionally we can ignore whenever there is no change
	if (encoderDelta == 0) return;
	
	//for some cases we only want to know if value is increased or decreased (typically for menu items)
	if (encoderDelta>0) Serial.print("+");
	if (encoderDelta<0) Serial.print("-");

	//for other cases we want to know what is current value. Additionally often we only want if something changed
	//example: when using rotary encoder to set termostat temperature, or sound volume etc
	
	//if value is changed compared to our last read
	if (encoderDelta!=0) {
		//now we need current value
		int16_t encoderValue = rotaryEncoder.readEncoder();
		//process new value. Here is simple output.
		Serial.print("Value: ");
		Serial.println(encoderValue);
	} 
}

void rotary_loop() {
	if (rotaryEncoder.currentButtonState() == BUT_RELEASED) {
    rotaryMode = nextEncoderMode(rotaryMode);
    Serial.print("Mode: ");
    Serial.println(rotaryMode);
	}

	int16_t encoderDelta = rotaryEncoder.encoderChanged();
	
	if (encoderDelta == 0) return;
	
	//for some cases we only want to know if value is increased or decreased (typically for menu items)
	if (encoderDelta > 0) //Encoder turned right
  {   
    switch(rotaryMode)
    {
      case LED_COLOR:
      default:
        hueUp();
        break;
      case LED_MODE:
        nextPattern();
        break;
      case LED_BRIGHTNESS:
        brightnessUp();
        break;
    }
  }

	if (encoderDelta < 0) //Encoder turned left
  {    switch(rotaryMode)
    {
      case LED_COLOR:
      default:
        hueDown();
        break;
      case LED_MODE:
        nextPattern();
        break;
      case LED_BRIGHTNESS:
        brightnessUp();
        break;
    }
  }

	//for other cases we want to know what is current value. Additionally often we only want if something changed
	//example: when using rotary encoder to set termostat temperature, or sound volume etc
	
	//if
	if (encoderDelta != 0) {
		//now we need current value
		int16_t encoderValue = rotaryEncoder.readEncoder();
		//process new value. Here is simple output.
		Serial.print("Value: ");
		Serial.println(encoderValue);
	} 
}

void setup() {
	Serial.begin(115200);
  delay(3000); // 3 second delay for recovery
  
  rotaryEncoder.begin();
	rotaryEncoder.setup([]{rotaryEncoder.readEncoder_ISR();});
  rotaryEncoder.setBoundaries(0, 10, true);

  pinMode(ARRAY_UP, INPUT_PULLUP);
  pinMode(ARRAY_DOWN, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  // EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically
  EVERY_N_MILLISECONDS( 20 ) { debounceArrayDown(); }
  EVERY_N_MILLISECONDS( 20 ) { debounceArrayUp(); } 
  EVERY_N_MILLISECONDS( 20 ) { rotary_loop(); } 
}