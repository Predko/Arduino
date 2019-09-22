/*
 * MIT License
 * 
 * Copyright (c) 2018 Leon van den Beukel
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Source: 
 * https://github.com/leonvandenbeukel/3D-7-Segment-Digital-Clock
 * 
 * External libraries you need:
 * Adafruit Sensor Library:   https://github.com/adafruit/Adafruit_Sensor
 * Adafruit RTCLib:           https://github.com/adafruit/RTClib
 * FastLED:                   https://github.com/FastLED/FastLED
 * Timer Library:             https://github.com/JChristensen/Timer
 * 
 * "Небольшие изменения внесены Предко Виктором"
 */
            


#include "DHT11_int.h"
#include <FastLED.h>
#include <Wire.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <Timer.h>

#include "AverageValue.h"
#include "SplitedString.h"


// DHT11
#define DHTPIN 12

DHTint dht(DHTPIN);

// LED
#define NUM_LEDS 29     
#define DATA_PIN 6

#define MINUTE_LO 0
#define MINUTE_HI 7

#define DOTS 14

#define HOUR_LO 15
#define HOUR_HI 22

// mode
#define CLOCK_MODE 0
#define TEMPERATURE_MODE 1
#define HUMIDITY_MODE 2
#define SCOREBOARD_MODE 3
#define TIMECOUNTER_MODE 4
#define CLOCK_TEMP_HUM_MODE 5

// Color mode
#define CRGB_MODE 0
#define CHSV_MODE 1
#define COLORCHNGPATTRN_MODE 2

// Dot mode - dotMode: 0=Both on, 1=Both Off, 2=Bottom On, 3=Blink
#define DOTS_BOTH_ON_MODE 0
#define DOTS_BOTH_OFF_MODE 1
#define DOTS_BOTTOM_ON_MODE 2
#define DOTS_BLINK_MODE 3

uint8_t dotMode = DOTS_BLINK_MODE;
CRGB LEDs[NUM_LEDS];

// Bluetooth
#define BT_PIN_RX 2
#define BT_PIN_TX 3

SoftwareSerial BTserial(BT_PIN_RX, BT_PIN_TX);
RTC_DS3231 rtc;
Timer t1;
Timer t2;
Timer t3;


CRGB colorCRGB = CRGB::MediumVioletRed;           // Change this if you want another default color, for example CRGB::Blue
CHSV colorCHSV = CHSV(95, 255, 255);  // Green
CRGB colorOFF  = CRGB::Black;      // Color of the segments that are 'disabled'. You can also set it to CRGB::Black
volatile uint8_t colorMODE = CHSV_MODE;           // 0=CRGB, 1=CHSV, 2=Constant Color Changing pattern
volatile uint8_t prevMode = -1;
volatile bool needRefresh = false;       
volatile uint8_t mode;                // 0=Clock, 1=Temperature, 2=Humidity, 3=Scoreboard, 4=Time counter
volatile int scoreLeft = 0;
volatile int scoreRight = 0;
volatile int timerValue = 0;
volatile bool timerRunning = false;

#define DOT_ON  true
#define DOT_OFF false

volatile bool dotOnOff;

#define hourFormat  24                // Set this to 12 or to 24 hour format
#define temperatureMode 'C'           // Set this to 'C' for Celcius or 'F' for Fahrenheit

AverageValue AverageTemperature(5);  // 5 последних измерений - примерно 5 минут 
AverageValue AverageHumidity(5);

SplitedString buffer;

void setup () {

  // Initialize LED strip
  FastLED.delay(3000);

  // Check if you're LED strip is a RGB or GRB version (third parameter)
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(LEDs, NUM_LEDS);

  Serial.begin(9600);
  while (!Serial) { /* Wait until serial is ready */ }

  BTserial.begin(9600);
  
  dht.begin();

  if (!rtc.begin()) {
    //Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    //Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  dotOnOff = DOT_ON;

  t1.every(1000 * 29, refreshT1); 
  t2.every(1000, refreshTimer);
  t3.every(50, updateHue);
  
  refreshDisplay();
  colorMODE = COLORCHNGPATTRN_MODE;
  mode = CLOCK_TEMP_HUM_MODE;
  //Serial.println(FastLED.getBrightness());
}

void loop () {
  
  t1.update(); 
  t2.update();
  t3.update();

  if(needRefresh && prevMode != -1)
  {
    refreshDisplay();
    needRefresh = false;
  }

  if (BTserial.available())
  {
    char received = BTserial.read(); //Serial.print("received = "); Serial.println(received);
    
    if (received == '|' || received == '.' || received == '\n')
    {
        processCommand();
    }
    else
    {
      buffer += received;
    }
    
  }
}

#define cmd_RGBD '0'                // , R,G,B,D
#define cmd_HSVD '1'                // , H,S,V,D
#define cmd_RTC  '2'                // , y, m, d, h, mm, s
#define cmd_CLOCK '3'
#define cmd_TEMPERATURE '4'
#define cmd_HUMIDITY '5'
#define cmd_SCOREBOARD '6'          // , scoreLeft, scoreRight
#define cmd_STARTTIMER '7'
#define cmd_STOPTIMER '8'
#define cmd_CHANGINGPATTERN '9'
#define cmd_CLOCK_TMP_HUM 'A'
#define cmd_SET_BRIGHTNESS 'B'      // , B
#define cmd_VARIABLE_BRIGHTNESS 'C' // , BR_min, BR_max

uint8_t DT[6][2] = // Day time brightness
{
  {10, 15},
  {10, 16},
  { 9, 17},
  { 7, 18},
  { 6, 19},
  { 5, 20}
};

uint8_t BR_min = 30,
        BR_max = 200;

bool modeVariableBrightness = true; // true - Variable Brightness

void processCommand(){

  if (buffer.GetFirstChar(0) == cmd_RGBD) 
  {
    int R = atoi(buffer.getValue(1));
    int G = atoi(buffer.getValue(2));
    int B = atoi(buffer.getValue(3));
    int D = atoi(buffer.getValue(4));
    colorCRGB.red = R;
    colorCRGB.green = G;
    colorCRGB.blue = B;
    colorMODE = CRGB_MODE;
    if (D > 0) FastLED.setBrightness(D); 
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_HSVD) 
  {
    int H = atoi(buffer.getValue(1));
    int S = atoi(buffer.getValue(2));
    int V = atoi(buffer.getValue(3));
    int D = atoi(buffer.getValue(4));
    colorCHSV.hue = H;
    colorCHSV.sat = S;
    colorCHSV.val = V;
    colorMODE = CHSV_MODE;
    if (D > 0) FastLED.setBrightness(D);
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_RTC) 
  {
    int y = atoi(buffer.getValue(1));
    int m = atoi(buffer.getValue(2));
    int d = atoi(buffer.getValue(3));
    int h = atoi(buffer.getValue(4));
    int mm = atoi(buffer.getValue(5));
    int s = atoi(buffer.getValue(6));
    rtc.adjust(DateTime(y, m, d, h, mm, s));
    Serial.println("DateTime set");
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_CLOCK) 
  {
    mode = CLOCK_MODE;
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_TEMPERATURE) 
  {
    mode = TEMPERATURE_MODE;    
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_HUMIDITY) 
  {
    mode = HUMIDITY_MODE;
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_SCOREBOARD) 
  {
    scoreLeft = atoi(buffer.getValue(1));
    scoreRight = atoi(buffer.getValue(2));
    mode = SCOREBOARD_MODE;    
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_STARTTIMER) 
  {
    timerValue = 0;
    timerRunning = true;
    mode = TIMECOUNTER_MODE;    
  } 
  else 
  if (mode == TIMECOUNTER_MODE && buffer.GetFirstChar(0) == cmd_STOPTIMER) 
  {
    timerRunning = false;
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_CHANGINGPATTERN) 
  {
    colorMODE = COLORCHNGPATTRN_MODE;
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_CLOCK_TMP_HUM) 
  {
    mode = CLOCK_TEMP_HUM_MODE;
  }
  else 
  if (buffer.GetFirstChar(0) == cmd_SET_BRIGHTNESS) 
  {
    uint8_t D = atoi(buffer.getValue(1));
    
    FastLED.setBrightness(D);
  } 
  else 
  if (buffer.GetFirstChar(0) == cmd_VARIABLE_BRIGHTNESS) 
  {
    if(modeVariableBrightness)
    {
      modeVariableBrightness = false;
      
      FastLED.setBrightness(BR_max);
    }
    else
    {
      uint8_t i = atoi(buffer.getValue(1));
      BR_min = (i != 0) ? i : BR_min;
      
      i = atoi(buffer.getValue(2));
      BR_max = (i != 0) ? i : BR_max;
      
      modeVariableBrightness = true;
    }
  } 
  
  buffer.Reset();

  refreshDisplay();
}


void updateHue() 
{
  if (colorMODE != COLORCHNGPATTRN_MODE)
  {
    return;
  }
     
  colorCHSV.sat = 255;
  colorCHSV.val = 255;
  if (colorCHSV.hue >= 255)
  {
    colorCHSV.hue = 0;
  } 
  else 
  {
    colorCHSV.hue++;
  }
  refreshDisplay();
}

#define FAILED_TO_READ_DHT "Failed to read from DHT sensor!"


void Collect_T_H() // Чтение данных сенсоров температуры и влажности
{
  int tmp = dht.readTemperature(temperatureMode == 'F' ? true : false);
  
  if (tmp == INT16_MIN) 
  {
    Serial.println(FAILED_TO_READ_DHT);
  } 
  else 
  {
    AverageTemperature.AddNext(tmp);
  }
  
  int hum = dht.readHumidity();
  
  if (hum == INT16_MIN) 
  {
    Serial.println(FAILED_TO_READ_DHT);
  } 
  else 
  {
    AverageHumidity.AddNext(hum);
  }
}

#define BEGIN_DAY 0
#define END_DAY   1

void refreshT1()
{
  if(modeVariableBrightness)
  {
    DateTime now = rtc.now();

    uint8_t m = now.month(),
            h = now.hour();

    m = (m <= 6) 
            ? m - 1 
            : 12 - m;

    if(h >= DT[m][BEGIN_DAY] && h <= DT[m][END_DAY])
    {
      FastLED.setBrightness(BR_max);
    }
    else
    {
      FastLED.setBrightness(BR_min);
    }
  }

  Collect_T_H();
  //Serial.println(rtc.getTemperature());
  refreshDisplay();
}

void refreshDisplay() {
  switch (mode) {
    case CLOCK_MODE:
      displayClock();
      break;
    case TEMPERATURE_MODE:
      displayTemperature();
      break;
    case HUMIDITY_MODE:
      displayHumidity();
      break;
    case SCOREBOARD_MODE:
      displayScoreboard();
      break;      
    case TIMECOUNTER_MODE:
      // Time counter has it's own timer
      break;
    case CLOCK_TEMP_HUM_MODE:
      displayClock();
      break;
    default:   
      break; 
  }
}



void refreshTimer() 
{
  if (mode == CLOCK_MODE || mode == CLOCK_TEMP_HUM_MODE) 
  {    
    dotOnOff = (dotOnOff == DOT_ON) ? DOT_OFF : DOT_ON;
    displayDots(dotMode);
  } 
  else 
  if (mode == TIMECOUNTER_MODE && timerRunning && timerValue < 6000) 
  {
    timerValue++;

    int m1 = (timerValue / 60) / 10 ;
    int m2 = (timerValue / 60) % 10 ;
    int s1 = (timerValue % 60) / 10;
    int s2 = (timerValue % 60) % 10;
  
    displaySegments(MINUTE_LO, s2); 
    displaySegments(MINUTE_HI, s1);
    displaySegments(HOUR_LO, m2);    
    displaySegments(HOUR_HI, m1);  
    displayDots(DOTS_BOTH_ON_MODE);  
    FastLED.show();
  }
}

void displayClock() 
{   
  DateTime now = rtc.now();

  int h  = now.hour();
  if (hourFormat == 12 && h > 12)
  {
    h = h - 12;
  }

  int hl = (h / 10) == 0 ? 13 : (h / 10);
  int hr = h % 10;
  int ml = now.minute() / 10;
  int mr = now.minute() % 10;

  displaySegments(MINUTE_LO, mr);    
  displaySegments(MINUTE_HI, ml);
  displaySegments(HOUR_LO, hr);    
  displaySegments(HOUR_HI, hl);  
  displayDots(dotMode);  
  FastLED.show();
}

void DHT11_NAN()
{
  LEDs[21] = LEDs[28] = ((colorMODE == CRGB_MODE) ? colorCRGB : colorCHSV);
}

void displayTemperature() 
{
  int tmp = AverageTemperature.Get();
  uint8_t tmp1, tmp2;

  if(tmp != INT16_MIN)
  {
    tmp  /=  10;
    tmp1 = tmp / 10;
    tmp2 = tmp % 10;
  }
  else
  {
    tmp1 = tmp2 = 15;
  }

  displaySegments(MINUTE_HI,  10);    
  displaySegments(MINUTE_LO, (temperatureMode == 'F' ? 14 : 11));
  displaySegments(HOUR_HI, tmp1);    
  displaySegments(HOUR_LO, tmp2);
  displayDots(DOTS_BOTH_OFF_MODE);  

  FastLED.show();    
}

void displayHumidity() 
{
  int hum = AverageHumidity.Get();
  uint8_t hum1, hum2;

  if(hum != INT16_MIN)
  {
    hum  /=  10;
  
    hum1 = hum / 10;
    hum2 = hum % 10;
  }
  else
  {
    hum1 = hum2 = 15;
  }

  displaySegments(MINUTE_HI,  10);    
  displaySegments(MINUTE_LO,  12);
  displaySegments(HOUR_HI, hum1);    
  displaySegments(HOUR_LO, hum2);
  displayDots(DOTS_BOTH_OFF_MODE);  

  FastLED.show();    
}

void displayScoreboard() {
  int s1 = scoreLeft % 10;
  int s2 = scoreLeft / 10;
  int s3 = scoreRight % 10;
  int s4 = scoreRight / 10;
  displaySegments(MINUTE_LO, s3);    
  displaySegments(MINUTE_HI, s4);
  displaySegments(HOUR_LO, s1);    
  displaySegments(HOUR_HI, s2);
  displayDots(DOTS_BOTTOM_ON_MODE);  
  FastLED.show();  
}

void displayDots(int dotMode) 
{

  if(mode == CLOCK_TEMP_HUM_MODE || prevMode != -1)
  {
    int8_t sec = rtc.now().second();
    if(sec > 20 && sec < 25)
    {
      if(prevMode == -1)
      {
        prevMode = mode;
        needRefresh = true;
      }
      
      mode = TEMPERATURE_MODE;
    }
    else
    if(sec > 30 && sec < 36)
    {
      if(prevMode == -1)
      {
        prevMode = mode;
        needRefresh = true;
      }
      
      mode = HUMIDITY_MODE;
    }
    else
    {
      if(prevMode != -1)
      {
        mode = prevMode;
        prevMode = -1;
        needRefresh = true;
      }
    }
  }

  // dotMode: 0=Both on, 1=Both Off, 2=Bottom On, 3=Blink
  switch (dotMode) {
    case DOTS_BOTH_ON_MODE:
      LEDs[DOTS] = colorMODE == CRGB_MODE ? colorCRGB : colorCHSV;
      break;
    case DOTS_BOTH_OFF_MODE:
      LEDs[DOTS] = colorOFF;
      break;
    case DOTS_BOTTOM_ON_MODE:
      LEDs[DOTS] = colorOFF;
      break;
    case DOTS_BLINK_MODE:
      LEDs[DOTS] = (dotOnOff == DOT_OFF) 
                              ? ((colorMODE == CRGB_MODE) ? colorCRGB : colorCHSV) 
                              : colorOFF;
      FastLED.show();  
      break;
    default:
      break;    
  }
}

void displaySegments(int startindex, int number) {

  byte numbers[] = {
    0b00111111, // 0    
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9   
    0b01100011, // ยบ              10
    0b00111001, // C(elcius)      11
    0b01011100, // ยบ lower        12
    0b00000000, // Empty          13
    0b01110001, // F(ahrenheit)   14
    0b01000000  // -              15
  };

  for (int i = 0; i < 7; i++) {
    LEDs[i + startindex] = ((numbers[number] & 1 << i) == 1 << i) 
                            ? (colorMODE == CRGB_MODE 
                                          ? colorCRGB 
                                          : colorCHSV) 
                            : colorOFF;
  } 
}
