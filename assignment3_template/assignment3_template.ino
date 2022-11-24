#include "M5CoreInk.h"
#include <Adafruit_NeoPixel.h>
#include "TimeFunctions.h"

Ink_Sprite PageSprite(&M5.M5Ink);
//Ink_Sprite TimeSprite(&M5.M5Ink);

RTC_TimeTypeDef RTCtime, RTCTimeSave;
RTC_TimeTypeDef AlarmTime;
uint8_t second = 0, minutes = 0;


#define PINK      0
#define ORANGE    1
#define YELLOW    2

#define NUMPIXELS 3
#define PIN       32 


// state constants:
const int STATE_DEFAULT       = 0;
const int STATE_EDIT_HOURS    = 1;
const int STATE_EDIT_MINUTES  = 2;
const int STATE_ALARM         = 4;
const int STATE_ALARMFINISHED = 5;
const int STATE_LED           = 6; 

// pin constants:
const int ledPin              = 10;             // built-in LED
const int sensorPin1          = 33;             // connector pin G33
const int sensorPin2          = 26;             // connector pin G26


// program states:
int program_state             = STATE_DEFAULT;  // initial program state is set to STATE_DEFAULT 
int led_state                 = PINK;

// timers:
unsigned long rtcTimer       = 0;   // real-time clock timer 
unsigned long beepTimer      = 0;       // real-time beep timer 
unsigned long underline_timer = 0;              // underline blink timer 
unsigned long sensorTimer    = 0;              // senser timer              
unsigned long led_timer       = 0;              // led timer              
bool underlineOn = false;


// variables:
int sensorVal1                = 0;
int sensorVal2                = 0;
int brightnessVal             = 0;

int PinkVal                   = 0;
int OrangeVal                 = 0;
int YellowVal                 = 0;


// pixels object to control the RGB LED Unit:
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(
                             NUMPIXELS,             // number of RGB LEDs
                             PIN,                   // pin the RGB LED Unit is connected to
                             NEO_GRB + NEO_KHZ800   // LED type
                           );


void setup() {
  M5.begin();                                     // initialize the M5 board
  pixels.begin();                                 // initialize the NeoPixel library
  
  M5.rtc.GetTime(&RTCTimeSave);
  AlarmTime = RTCTimeSave;
  AlarmTime.Minutes = AlarmTime.Minutes + 2;      // set alarm 2 minutes ahead
  M5.update();
  
  checkRTC();
  PageSprite.creatSprite(0, 0, 200, 200);
  drawTime();
  
  M5.M5Ink.clear();
  delay(500);
  
  pixels.begin ();
  pixels.show();
  pixels.setBrightness(50);

  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);
  
  Serial.begin(9600);                             // open Serial port at 9600 bit per second
  
  digitalWrite(ledPin, LOW);                      // turn on built-in led
  M5. begin(true,true, true) ;                    // alarm beeping


  
}

void loop() {
  if (Serial.available() > 0)
  {
    // input format is "hh:mm"
    char input[6];
    int charsRead = Serial.readBytes(input, 6);    // read 6 characters
    if (charsRead == 6 && input[2] == ':') {
      int mm = int(input[4] - '0') + int(input[3] - '0') * 10;
      Serial.print("minutes: ");
      Serial.println(mm);
      int hr = int(input[1] - '0') + int(input[0] - '0') * 10;
      Serial.print("hours: ");
      Serial.println(hr);
      RTCtime.Minutes = mm;
      RTCtime.Hours = hr;
      M5.rtc.SetTime(&RTCtime);
    }
    else {
      Serial.print("received wrong time format.. ");
      Serial.println(input);
    }
  }

  //if((program_state != STATE_EDIT_HOURS) && (program_state != STATE_EDIT_MINUTES)) {
  if ( program_state == STATE_DEFAULT) {
    
    if(millis() > beepTimer + 1000){
      M5.Speaker.tone(500,300) ;
      beepTimer = millis();
      updateTime();
      drawTime();
      drawTimeToAlarm();
      rtcTimer = millis();
    }

        // state behavior: check and update time every second
    if(millis() > rtcTimer + 1000) {
      updateTime();
      drawTime();
      drawTimeToAlarm();      
      rtcTimer = millis();
    }
    // state behavior: read sensor and print its value to Serial port
    if(millis() > sensorTimer + 100) {
       
      sensorVal1 = analogRead(sensorPin1);
      brightnessVal = map(sensorVal1, 0, 4095, 0, 255);
      Serial.println(brightnessVal);
      
      //analogWrite(ledPin, brightnessVal);
      
      sensorVal2 = analogRead(sensorPin2);
      
      pixels.setPixelColor(0, pixels.Color(brightnessVal, 100, 100));
      pixels.setPixelColor(2, pixels.Color(PinkVal, OrangeVal, YellowVal));
      pixels.show();
      
      sensorTimer = millis();  
    }
      
    // state behavior: check and update time every 100ms
    if (millis() > rtcTimer + 100) {
      Serial.print("sensorVal1 = ");
      Serial.println(sensorVal1);
      Serial.print("sensorVal2 = ");
      Serial.println(sensorVal2);
      Serial.print("brightnessVal = ");
      Serial.println(brightnessVal);
      
      updateTime();
      drawTime(); 
      
      rtcTimer = millis();
    }
    
    // For every 1000 ms
    if(millis() > led_timer + 1000){
      if (led_state == PINK) {
        PinkVal = 0;
        OrangeVal = 255;
        YellowVal = 0;
        led_state = ORANGE;
      } else if(led_state == ORANGE) {
        PinkVal = 0;
        OrangeVal = 0;
        YellowVal = 255;
        led_state = YELLOW; 
      } else if(led_state == YELLOW) {
        PinkVal = 255;
        OrangeVal = 0;
        YellowVal = 0;
        led_state = PINK; 
      }
      led_timer = millis();
    }
    
    // state transition:
    if ( M5.BtnMID.wasPressed()) {
      AlarmTime = RTCtime;
      program_state = STATE_EDIT_MINUTES; 
      Serial.println("program_state => STATE_EDIT_MINUTES");
    }
    // state transition: alarm time equals real time clock 
    if(AlarmTime.Hours == RTCtime.Hours && AlarmTime.Minutes == RTCtime.Minutes) {
      program_state = STATE_ALARM;
      Serial.println("program_state => STATE_ALARM");
    }
  }
  else if ( program_state == STATE_EDIT_MINUTES) {
    // state behavior:
    if ( millis() > underline_timer + 250 ) {
      underlineOn = !underlineOn;
      PageSprite.drawString(30, 20, "SET ALARM MINUTES:");
      //drawTime(&AlarmTime);
      drawAlarmTime();
      if (underlineOn)
        PageSprite.FillRect(110, 110, 80, 6, 0); // underline minutes black
      else
        PageSprite.FillRect(110, 110, 80, 6, 1); // underline minutes white
      PageSprite.pushSprite();
      underline_timer = millis();
    }
    // state transition: UP button
    if ( M5.BtnUP.wasPressed()) {
      Serial.println("BtnUP wasPressed!");
      if (AlarmTime.Minutes < 60) {
        AlarmTime.Minutes++;
        //drawTime(&AlarmTime);
        drawAlarmTime();
        PageSprite.pushSprite();
      }
    }
    // another state transition: DOWN button
    else if ( M5.BtnDOWN.wasPressed()) {
      Serial.println("BtnDOWN wasPressed!");
      if (AlarmTime.Minutes > 0) {
        AlarmTime.Minutes--;
        //drawTime(&AlarmTime);
        drawAlarmTime();
        PageSprite.pushSprite();
      }
    }
    // another state transition: MID button
    else if (M5.BtnMID.wasPressed()) {
      PageSprite.FillRect(110, 110, 80, 6, 1); // underline minutes white
      program_state = STATE_EDIT_HOURS;
      Serial.println("program_state => STATE_EDIT_HOURS");
    }
  }
  else if (program_state == STATE_EDIT_HOURS) {

    if (millis() > underline_timer + 250) {
      PageSprite.drawString(30, 20, " SET ALARM HOURS: ");
      underlineOn = !underlineOn;
      drawTime();
      //drawTime(&AlarmTime);
      drawAlarmTime();
      if (underlineOn)
        PageSprite.FillRect(10, 110, 80, 6, 0); // underline hours black
      else
        PageSprite.FillRect(10, 110, 80, 6, 1); // underline hours white
      PageSprite.pushSprite();
      underline_timer = millis();
    }

    if ( M5.BtnUP.wasPressed()) {
      Serial.println("BtnUP wasPressed!");
      if (AlarmTime.Hours < 24) {
        AlarmTime.Hours++;
        //drawTime(&AlarmTime);
        drawAlarmTime();
        PageSprite.pushSprite();
      }
    }
    else if ( M5.BtnDOWN.wasPressed()) {
      Serial.println("BtnDOWN wasPressed!");
      if (AlarmTime.Hours > 0) {
        AlarmTime.Hours--;
        //drawTime(&AlarmTime);
        drawAlarmTime();
        PageSprite.pushSprite();
      }
    }
    else if (M5.BtnMID.wasPressed()) {
      //PageSprite.FillRect(10, 110, 80, 6, 1); // underline hours white
      M5.M5Ink.clear();
      PageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
      M5.rtc.GetTime(&RTCtime);
      //drawTime(&RTCtime);
      drawTime();
      PageSprite.pushSprite();

      program_state = STATE_DEFAULT;
      Serial.println("program_state => STATE_DEFAULT");
    }
    
  }  
    // program state alarm sounds
  
  else if (program_state == STATE_ALARM) {
    
    if(millis() > rtcTimer + 1000) {
      updateTime();
      drawTime();
      drawTimeToAlarm();      
      rtcTimer = millis();
    }
    if (M5.BtnEXT.wasPressed()) {
      Serial.println("BtnEXT wasPressed!");
      M5.M5Ink.clear();
      PageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
      program_state = STATE_ALARMFINISHED;
      Serial.println("program_state => STATE_ALARM_FINISHED");
    }
    
    if (sensorVal1 > 400) { //analogRead(sensorPin1)) {
      Serial.println("sensor value was exceeded");
      M5.M5Ink.clear();
      PageSprite.clear(CLEAR_DRAWBUFF | CLEAR_LASTBUFF);
      program_state = STATE_ALARMFINISHED; 
      Serial.println("program_state => STATE_ALARM_FINISHEED");
    }


  }
  
 // program state alarm turn off
  else if (program_state == STATE_ALARMFINISHED) {
    // state behavior: check and update time every second
    if(millis() > rtcTimer + 1000) {
      updateTime();
      drawTime();
      PageSprite.drawString(50, 120, "ALARM IS OFF");
      PageSprite.pushSprite();    
      rtcTimer = millis();
    }
    // state transition: MID button 
    if ( M5.BtnMID.wasPressed()) {
      AlarmTime = RTCtime;
      program_state = STATE_EDIT_MINUTES;
      Serial.println("program_state => STATE_EDIT_MINUTES");
    }
    
    if (sensorVal2 > 400) {
      Serial.println("turn off LEDs..");
      for (int i=0; i<3; i++)
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
    }
  }
  
  
  
// program state sensor turn off

  //else if (program_state == STATE_LED) {
  //  if (sensorVal2 > 400) {
   //   Serial.println("turn off the LED..");
       
   // }
    
    
    
//  }
  

  M5.update();
}

