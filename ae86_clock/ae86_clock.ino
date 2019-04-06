#if defined(TEENSYDUINO)
#define IS_TEENSY
#elif defined(ARDUINO)
#define IS_ARDUINO
#elif defined(STM32F3)
#define IS_STM32
#elif defined(ESP8266)
#define IS_ESP
#endif

#define DEBUG
//#define NO_LOGO
#define ONEWIRETEMP

#if defined(IS_ESP)
#define _PROGMEM
#else
#define _PROGMEM PROGMEM
#endif

#include <U8g2lib.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#ifndef ONEWIRETEMP
#include <SHT21.h>
#else
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#include <Sodaq_DS3231.h>
#include <EEPROM.h>
#include "fonts.h"
#include "strings.h"
#include "globals.h"
#include "boot_logos.h"
#include "functions.h"

void setup() {
  u8g2.begin();
  Wire.setClock(4000000);
  rtc.begin();

  DateTime Clock = rtc.now();
  workingDay = max(0, Clock.date() - 1);
  workingMonth = max(0, Clock.month() - 1);
  workingYear = max(2000, Clock.year() - 1);
  workingNDay = Clock.dayOfWeek();
  workingHour = Clock.hour();
  workingMin = Clock.minute();
#ifdef DEBUG
  Serial.begin(115200);
#endif

  pinMode(BTN_SET, INPUT_PULLUP);  // Set Button
  pinMode(BTN_MIN, INPUT_PULLUP);  // + Button
  pinMode(BTN_HOUR, INPUT_PULLUP); // - Button
  pinMode(LED_BUILTIN, OUTPUT); // Status LED

#if defined(IS_TEENSY)
  analogReadRes(12);
  analogReadAveraging(30);
#endif
#if defined(IS_STM32)
  analogReadResolution(12);
#endif

#ifdef ONEWIRETEMP
  temperature.begin();
#endif

  // Load Config from EEPROM
  readConfig();

  u8g2.setContrast(5 + (settings.displayBrightness * 25));

  u8g2.clearBuffer();
#ifndef NO_LOGO
  u8g2.drawXBMP(0, 0, 128, 32, boot_logo);
#else
  u8g2.setFont(Logisoso_Custom);
  printCenter(PSTR("AE86"), 128, 22);
#endif
  u8g2.sendBuffer();
 
  delay(3000);
}

void loop() {
  checkButtons();
  checkPane();
}

void checkPane()
{
  u8g2.clearBuffer();
  u8g2.setFont(Logisoso_Custom);

  if(enterTimeSettings)
    settingTime();
  else if(enterSettings)
  {
    if(!transCfg) memcpy(&tempSettings, &settings, sizeof(EEPROM_SETTINGS));

    switch(displayMenu("SETTINGS", strSetting, SETTINGS_END, &settingsSelect, &enterSettings))
    {
      case SETTINGS_TIME:
        settingTime();
        break;
      case SETTINGS_TTYPE:
        settingClockType();
        break;
      case SETTINGS_TEMP:
        settingTemp();
        break;
      case SETTINGS_VOLT:
        settingVolt();
        break;
      case SETTINGS_BRIGHTNESS:
        settingBrightness();
        break;
      case SETTINGS_PANE:
        settingPane();
        break;
      case SETTINGS_SAVE:
        settingSave();
        break;
      case SETTINGS_QUIT:
        settingQuit();
        break;
      default:
        break;
    }
  }
  else
  {
    sampleVolts += analogRead(VOLT_IN);
    voltSampleCount++;
    if(voltSampleCount >= VOLTAGE_SAMPLE)
    {
      volt = GetADC(((float)sampleVolts / (float)VOLTAGE_SAMPLE)) / (R2 / (R1 + R2));
      voltSampleCount = 0;
      sampleVolts = 0;
    }
    
    if(volt <= settings.voltWarn)
      u8g2.drawXBMP(117, 23, 11, 8, bat_logo);
    if(isIce())
      u8g2.drawXBMP(117, 10, 11, 7, ice_logo);
    if(settingsHold)
      u8g2.drawBox(2, (30 / SETTINGS_ENTR) * (SETTINGS_ENTR - settingsHold), 2, (30 / SETTINGS_ENTR) * settingsHold);
    switch(paneSelect)
    {
      case PANE_TIME:
        paneTime();
        break;
      case PANE_TEMP:
        paneTemp();
        break;
      case PANE_VOLT:
        paneVolt();
        break;
      default:
        paneTime();
        break;
    }
    if((millis() % 0xFFFFFFFF) - timer > 1000)
    {
      timer = millis() % 0xFFFFFFFF;
      counter = (counter + 1) % 30;
      LEDTOGGLE = ~LEDTOGGLE;
    }
  }
  u8g2.sendBuffer();
}

