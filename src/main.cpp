/*
 *  @author   Tauno Erik
 *  @github
 *  @website  https://taunoerik.art/
 *  @started  19.09.2019
 */
#include <Arduino.h>
#include <math.h>               // calculating temp
#include "pitches.h"            // notes
#include <Adafruit_NeoPixel.h>

/* LED strip */
const unsigned char led_count = 72;
unsigned char led_brightness = 50;  // NeoPixel brightness, 0 (min) to 255 (max)

unsigned char red   = 0;
unsigned char green = 0;
unsigned char blue  = 0;
unsigned char white = 50;

/* LED display mode */
unsigned char mode = 0;  // 0 - mormal, any other nr. - special mode
unsigned int party_mode_delay = 10*1000;

boolean party_play = true;


/* Button press interval */
boolean long_button_press = false;
unsigned long previous_millis_measure = 0;
unsigned int measure_interval = 9000;

/* PINS */
const int thermistor_pin = A3;
const int piezo_pin      = A5;
const int led_strip_pin  = 6;

/* Buttons */
const unsigned char btn_pin_1 = 8;   // PCINT0
const unsigned char btn_pin_2 = 9;   // PCINT0
const unsigned char btn_pin_3 = 10;  // PCINT0
const unsigned char btn_pin_4 = 11;  // PCINT0
const unsigned char btn_pin_5 = 12;  // PCINT0
const unsigned char btn_pin_6 = 7;   // PCINT2
const unsigned char btn_pin_7 = 4;   // PCINT2
const unsigned char btn_pin_8 = 3;   // INT1
const unsigned char btn_pin_9 = 2;   // INT0

// http://gammon.com.au/interrupts
// In priority order:
// 2 External Interrupt Request 0   (pin D2)         (INT0_vect)
// 3 External Interrupt Request 1   (pin D3)         (INT1_vect)
// 4 Pin Change Interrupt Request 0 (pins D8 to D13) (PCINT0_vect)
// 5 Pin Change Interrupt Request 1 (pins A0 to A5)  (PCINT1_vect)
// 6 Pin Change Interrupt Request 2 (pins D0 to D7)  (PCINT2_vect)

/* Init LED stripe */
Adafruit_NeoPixel strip(led_count, led_strip_pin, NEO_RGBW + NEO_KHZ800);
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


/**************************************
 ***  FUNCTIONS ***********************
 **************************************/

/*!
 *  @brief    Read analog pin and calculate temperature
 *  @return   Temperature in Celcius
 *  @note     10K thermistor + 10K resistor
 */
float measure_temp() {
  // https://arduinomodules.info/ky-013-analog-temperature-sensor-module/
  int Vo{};
  float logR2, R2, T;
  const float R1 = 10000; // value of R1 on board 10K
  const float c1 = 0.001129148, c2 = 0.000234125, c3 = 0.0000000876741; //steinhart-hart coeficients for thermistor

  Vo = analogRead(thermistor_pin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0); //calculate resistance on thermistor
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2)); // temperature in Kelvin
  T = T - 273.15; //convert Kelvin to Celcius
 // T = (T * 9.0)/ 5.0 + 32.0; //convert Celcius to Farenheit

  return T;
}


/*!
 * @brief   Play note
 * @param   speaker_pin
 * @param   the_tone
 *          define on pitches.h
 * @param   duration
 *          1 to 7
 */
void play_note(uint8_t speaker_pin, uint16_t the_tone, uint8_t duration) {
  long time = 500 / pow(2, duration - 1);
  tone(speaker_pin, the_tone, time);
}


/*!
 * @brief   Checks whether the button is pressed down or not.
 * @param   pin
 *          Button pin is pulledup (HIGH)
 * @return  True if is pressed down (LOW), else false
 */
bool is_button(uint8_t pin) {
  if (digitalRead(pin) == LOW) {
    delay(45);

    if (digitalRead(pin) == LOW) {      
      delay(45);
      return true; //kui nupp on all
    }
    return false;
  }
  return false;
}


/*!
 *  @brief    Increase color brightness
 *  @param    Current color value 0 -255
 *  @return   New color value 0 -255
 */
unsigned char increase_color_brightness(unsigned char color) {
  if (color < 255) {
    if (long_button_press && color < 240) {
      color = color + 10;
    }
    else {
      color++;
    }
  }
  return color;
}


/*!
 *  @brief    Increase color brightness
 *  @param    Current color value 0 -255
 *  @return   New color value 0 -255
 */
unsigned char decrease_color_brightness(unsigned char color) {
  if (color > 0) {
    if (long_button_press && color > 10) {
      color = color - 10;
    }
    else {
      color--;
    }
  }
  return color;
}


/*!
 *  @brief  Default LED display mode.
 *          User can manually select any color combination
 *  @note   Mode 0
 */
void default_led_mode() {
  strip.fill(strip.Color(green, red, blue, white));
  strip.show();
  //delay(5);
}


/*!
 *  @brief  Rainbow LED display mode.
 *  @note   Mode 1
 */
void party_led_mode(int wait) {
    // Hue of first pixel runs 3 complete loops through the color wheel.
    // Color wheel has a range of 65536 but it's OK if we roll over, so
    // just count from 0 to 3*65536. Adding 256 to firstPixelHue each time
    // means we'll make 3*65536/256 = 768 passes through this outer loop:
  if(party_play) {
    for(long firstPixelHue = 0; firstPixelHue < 3*65536; firstPixelHue += 256) {
      if (mode != 1){ break; }
      for(int i=0; i<strip.numPixels(); i++) { 
        if (mode != 1){ break; }
        // For each pixel in strip...
        // Offset pixel hue by an amount to make one full revolution of the
        // color wheel (range of 65536) along the length of the strip
        // (strip.numPixels() steps):
        int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
        // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
        // optionally add saturation and value (brightness) (each 0 to 255).
        // Here we're using just the single-argument hue variant. The result
        // is passed through strip.gamma32() to provide 'truer' colors
        // before assigning to each pixel:
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
      }
      strip.show(); // Update strip with new contents
      //delay(wait);  // Pause for a moment
    }
  }
}


/*
 *  @brief  Special button interrupt 
 *  @note   Button: btn_pin_9
 */
void special_btn_pressed() {
  if(mode == 0) {
    mode++;
    play_note(piezo_pin, NOTE_DS8, 4);
  }
  else{
    mode = 0;
    play_note(piezo_pin, NOTE_D8, 4);
    // TODO: reset!?
  }
}

/*******************************************************************************/

void setup() {

  /* LED strip */
  strip.begin();                        // INITIALIZE NeoPixel strip object
  strip.show();                         // Turn OFF all pixels ASAP
  strip.setBrightness(led_brightness);

  /* Buttons */
  pinMode(btn_pin_1, INPUT_PULLUP);
  pinMode(btn_pin_2, INPUT_PULLUP);
  pinMode(btn_pin_3, INPUT_PULLUP);
  pinMode(btn_pin_4, INPUT_PULLUP);
  pinMode(btn_pin_5, INPUT_PULLUP);
  pinMode(btn_pin_6, INPUT_PULLUP);
  pinMode(btn_pin_7, INPUT_PULLUP);
  pinMode(btn_pin_8, INPUT_PULLUP);
  pinMode(btn_pin_9, INPUT_PULLUP);

  // Interrupt types: 
  // LOW      The low level of INT0 generates an interrupt request
  // CHANGE   Any logical change on INT0 generates an interrupt request
  // FALLING  The falling edge of INT0 generates an interrupt request
  // RISING   The rising edge of INT0 generates an interrupt request
  attachInterrupt (digitalPinToInterrupt (btn_pin_9), special_btn_pressed, RISING);  // attach interrupt handler

  Serial.begin(9600);
}

/***********************************************************************************/

void loop() {
  start:

  unsigned long current_millis = millis();

  // Long on short button press? 
  if ((unsigned long)(current_millis - previous_millis_measure) >= measure_interval) {
    long_button_press = true;
  }
  else {
    long_button_press = false;
  }

  // party mode
  if ((unsigned long)(current_millis - previous_millis_measure) >= party_mode_delay) {
    party_play = true;
  }
  else {
    party_play = false;
  }


  // Normal or special mode? User selects.
  switch (mode) {
    case 0:
      default_led_mode();
      break;
    default:
      party_led_mode(party_mode_delay);
      break;
  }


  // Which button is pressed
  if (is_button(btn_pin_1)) {
    white = increase_color_brightness(white);
    Serial.print("W + "); Serial.print(white); Serial.print(" ");
    Serial.print(measure_temp()); Serial.println(" C");
  }
  else if (is_button(btn_pin_2)) {
    white = decrease_color_brightness(white);
    Serial.print("W - ");Serial.print(white);Serial.print(" ");
    Serial.print(measure_temp());Serial.println(" C");
  }
  else if (is_button(btn_pin_3)) {
    red = increase_color_brightness(red);
    Serial.print("R + ");Serial.print(red);Serial.print(" ");
    Serial.print(measure_temp());Serial.println(" C");
  }
  else if (is_button(btn_pin_4)) {
    red = decrease_color_brightness(red);
    Serial.print("R - ");Serial.print(red);Serial.print(" ");
    Serial.print(measure_temp());Serial.println(" C");
  }
  else if (is_button(btn_pin_5)) {
    green = increase_color_brightness(green);
    Serial.print("G + ");Serial.print(green);Serial.print(" ");
    Serial.print(measure_temp());Serial.println(" C");
  }
  else if (is_button(btn_pin_6)) {
    green = decrease_color_brightness(green);
    Serial.print("G - ");Serial.print(green);Serial.print(" ");
    Serial.print(measure_temp());Serial.println(" C");
  }
  else if (is_button(btn_pin_7)) {
    blue = increase_color_brightness(blue);
    Serial.print("B + ");Serial.print(blue);Serial.print(" ");
    Serial.print(measure_temp());Serial.println(" C");
  }
  else if (is_button(btn_pin_8)) {
    blue = decrease_color_brightness(blue);
    Serial.print("B - ");Serial.print(blue);Serial.print(" ");
    Serial.print(measure_temp());Serial.println(" C");
  }

  // btn_pin_9 is special interrrupt button!
  
} // Main Loop end