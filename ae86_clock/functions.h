int8_t displayMenu(const char * title, const char ** items, uint8_t numItems, int8_t* selection, uint8_t* active);
uint32_t getRPM();
uint32_t getSPD();
void clearConfig();

#ifdef ONEWIRETEMP
float getTemp(uint8_t temptype)
{
  temperature.requestTemperatures();
  return (temptype ? temperature.getTempFByIndex(0) : temperature.getTempCByIndex(0));
}
#endif

void printCenter(char* str, u8g2_uint_t scrWidth, u8g2_uint_t yPos)
{
  int16_t offset = u8g2.getStrWidth(str);
  u8g2.drawStr((scrWidth / 2) - (offset / 2), yPos, str);
  u8g2.sendBuffer();
}

int8_t checkButton(uint8_t button)
{
  if((digitalRead(button) == LOW) && ((millis() - lastDebounceTime) > DEBOUNCE))
  {
    lastDebounceTime = millis();
    return 1;
  }
  else if (digitalRead(button) == HIGH)
    return 0;
  else
    return -1;
}

void checkButtons()
{
  if(checkButton(BTN_SET) == 1)
  {
    if(!enterSettings)
    {
      settingsHold++;
      if(settingsHold > SETTINGS_ENTR)
      {
        enterSettings = 1;
        settingsHold = 0;
        //clearConfig();
      }
    }
  }
  
  if(checkButton(BTN_MIN) == 1)
  {
    paneSelect--;
    if(paneSelect < 0) paneSelect = PANE_END - 1;
  }
  
  if(checkButton(BTN_HOUR) == 1)
  {
    paneSelect = (paneSelect + 1) % PANE_END;
  }
  if((checkButton(BTN_SET) == 0) && (settingsHold)) settingsHold = 0;
}

void _setTime()
{
  char tempStr[20];
  uint8_t changed = 1;
  uint8_t exit = 0;
  DateTime Clock = rtc.now();
  workingHour = Clock.hour();
  workingMin = Clock.minute();

  u8g2.setFont(Logisoso_Custom);
  
  while(!exit)
  {
    if(checkButton(BTN_MIN) == 1)
    {
      workingMin = (workingMin + 1) % 60;
      changed = 1;
    }
    if(checkButton(BTN_HOUR) == 1)
    {
      workingHour = (workingHour + 1) % 24;
      changed = 1;
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      exit = 1;
    }
    if(changed)
    {
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("%02d"), workingHour);
      u8g2.drawStr(20, 31, tempStr);
      sprintf_P(tempStr, PSTR("%02d"), workingMin);
      u8g2.drawStr(70, 31, tempStr);
      u8g2.drawStr(59, 28, ":");
      u8g2.sendBuffer();
      changed = 0;
    }
  }
}

void _setDate()
{
  char tempStr[20];
  uint8_t exit = 0;
  uint8_t type = SETTING_DATE_DAY;
  DateTime Clock = rtc.now();
  workingDay = max(0, Clock.date() - 1);
  workingMonth = max(0, Clock.month() - 1);
  workingYear = max(2000, Clock.year() - 1);
  workingNDay = Clock.dayOfWeek();
  uint16_t* workingValue;
  uint16_t limit = 0;

  while(!exit)
  {
    switch(type)
    {
      case SETTING_DATE_DAY:
        workingValue = &workingDay;
        limit = 31;
        break;
      case SETTING_DATE_MTH:
        workingValue = &workingMonth;
        limit = 12;
        break;
      case SETTING_DATE_YR:
        workingValue = &workingYear;
        limit = 2100;
        break;
      case SETTING_DATE_DOW:
        workingValue = &workingNDay;
        limit = 7;
        break;
    }
    if(checkButton(BTN_MIN) == 1)
    {
      if(*workingValue == 0) *workingValue = limit - 1;
      else *workingValue = *workingValue - 1;
    }
    if(checkButton(BTN_HOUR) == 1)
    {
      *workingValue = (*workingValue + 1) % limit;
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      if(type < SETTING_DATE_END - 1)
        type++;
      else
        exit = 1;
    }
    
    {
      u8g2.clearBuffer();
      if(type == SETTING_DATE_DAY)
        sprintf_P(tempStr, PSTR("[%02d]/%02d/%04d"), workingDay+1, workingMonth+1, workingYear+1);
      else if(type == SETTING_DATE_MTH)
        sprintf_P(tempStr, PSTR("%02d/[%02d]/%04d"), workingDay+1, workingMonth+1, workingYear+1);
      else if(type == SETTING_DATE_YR)
        sprintf_P(tempStr, PSTR("%02d/%02d/[%04d]"), workingDay+1, workingMonth+1, workingYear+1);
      else
        sprintf_P(tempStr, PSTR("Day: %S"), (char*)pgm_read_word(&(days[workingNDay])));
      printCenter(tempStr, 128, 22);
      u8g2.sendBuffer();
    }
  }
}

void _setTimeSave()
{
  DateTime dt(workingYear + 1, workingMonth + 1, workingDay + 1, workingHour, workingMin, 0, workingNDay);
  rtc.setDateTime(dt);
  enterTimeSettings = 0;
  timeSelect = 0;
}

void paneTime()
{
  char tempStr[20];
  DateTime Clock = rtc.now();
  uint8_t workingHour = settings._24h ? ((Clock.hour() == 12) ? 12 : (Clock.hour() % 12)) : Clock.hour();
  if(settings._24h)
  {
    sprintf_P(tempStr, PSTR("%d"), workingHour);
  }
  else
  {
    sprintf_P(tempStr, PSTR("%02d"), workingHour);
  }
  u8g2.drawStr((settings._24h && workingHour < 10) ? 41 : 20, 31, tempStr);
  sprintf_P(tempStr, PSTR("%02d"), Clock.minute());
  u8g2.drawStr(70, 31, tempStr);
  if(counter % 2)
    u8g2.drawStr(59, 28, ":");
}

void paneTemp()
{
  char tempStr[20];
  uint16_t offset = 0, offsetBase = 0;
  float temp = 0.0, hum = 0.0;
#ifndef ONEWIRETEMP
  temp = sht21.getTemperature(settings.tempType);
  hum = sht21.getHumidity();
#else
  temp = getTemp(settings.tempType);
#endif
  sprintf_P(tempStr, PSTR("999\xb0"), NULL);
  offsetBase = u8g2.getStrWidth(tempStr);
  sprintf_P(tempStr, PSTR("%d\xb0"), (int)temp);
  offset = u8g2.getStrWidth(tempStr);
  u8g2.drawStr(30 + (offsetBase - offset), 31, tempStr);
  sprintf_P(tempStr, PSTR("%S"), (char*)pgm_read_word(&(tempType[settings.tempType])));
  u8g2.setFont(u8g2_font_calibration_gothic_nbp_tr);
  u8g2.drawStr(100, 31, tempStr);
#ifndef ONEWIRETEMP
  sprintf_P(tempStr, PSTR("%d%%"), (int)hum);
  u8g2.drawStr(0, 32, tempStr);
  u8g2.drawStr(0, 17, "HUM");
#endif
}
        
void paneVolt()
{
  char tempStr[20];
  uint8_t vLeft = volt;
  uint8_t vRight = (uint8_t)(floor(volt * 10)) % 10;
  sprintf_P(tempStr, PSTR("%d.%dv"), vLeft, vRight);
  printCenter(tempStr, 128, 31);
}

#ifndef BASIC
void paneRPM()
{
  char tempStr[20];
  if(millis() - previousMillis > REFRESH_INT)
  {
    previousMillis = millis();
    engineRpm = getRPM();
  }
  sprintf_P(tempStr, PSTR("%d"), engineRpm);
  uint16_t offsetBase = u8g2.getStrWidth("9999");
  uint16_t offset = u8g2.getStrWidth(tempStr);
  u8g2.drawStr(8 + (offsetBase - offset), 31, tempStr);
  strcpy_P(tempStr, STR_ACKRPM);
  u8g2.drawStr(84, 31, tempStr);
}
        
void paneSpeed()
{
  char tempStr[20];
  uint16_t offset = 0, offsetBase = 0;
  sprintf_P(tempStr, PSTR("%d"), carSpeed);
  offsetBase = u8g2.getStrWidth("999");
  offset = u8g2.getStrWidth(tempStr);
  u8g2.drawStr(27 + (offsetBase - offset), 31, tempStr);
  strcpy_P(tempStr, (char*)pgm_read_word(&(speed_acr[settings.speedAcr])));
  u8g2.drawStr(84, 31, tempStr);

  if(millis() - previousMillis > REFRESH_INT)
  {
    previousMillis = millis();
    carSpeed = getSPD();
  }
}
#endif

void settingTime()
{
  uint8_t exit = 0;
  char tempStr[20];

  switch(displayMenu((const char *)NULL, strTimeSetting, SETTING_TIME_END, &timeSelect, &enterTimeSettings))
    {
      case SETTING_TIME_TIME:
        _setTime();
        break;
      case SETTING_TIME_DATE:
        _setDate();
        break;
      case SETTING_TIME_SAVE:
        _setTimeSave();
        break;
      case SETTING_TIME_BACK:
        enterTimeSettings = 0;
        timeSelect = 0;
        break;
      default:
        break;
    }
}

void settingClockType()
{
  char tempStr[20];
  uint8_t exit = 0, lastSelect = tempSettings._24h;

  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Type: %S"), (char*)pgm_read_word(&(clockType[tempSettings._24h])));
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  { 
    if((checkButton(BTN_MIN) == 1) || (checkButton(BTN_HOUR) == 1))
      tempSettings._24h = (tempSettings._24h ^ 1) & 1;
    
    if(lastSelect != tempSettings._24h)
    {
      lastSelect = tempSettings._24h;
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Type: %S"), (char*)pgm_read_word(&(clockType[tempSettings._24h])));
      printCenter(tempStr, 128, 22);
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}

#ifndef BASIC
void settingSpdType()
{
  char tempStr[20];
  
  uint8_t exit = 0, lastSelect = tempSettings.speedAcr;

  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Speed: %S"), (char*)pgm_read_word(&(spdType[tempSettings.speedAcr])));
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  {
    if((checkButton(BTN_MIN) == 1) || (checkButton(BTN_HOUR) == 1))
      tempSettings.speedAcr = (tempSettings.speedAcr ^ 1) & 1;
    
    if(lastSelect != tempSettings.speedAcr)
    {
      lastSelect = tempSettings.speedAcr;
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Speed: %S"), (char*)pgm_read_word(&(spdType[tempSettings.speedAcr])));
      printCenter(tempStr, 128, 22);
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}
#endif

void settingTemp()
{
  char tempStr[20];
  uint8_t exit = 0, lastSelect = tempSettings.tempType;

  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Temp: %S"), (char*)pgm_read_word(&(tempType[tempSettings.tempType])));
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  {
    if((checkButton(BTN_MIN) == 1) || (checkButton(BTN_HOUR) == 1))
      tempSettings.tempType = (tempSettings.tempType ^ 1) & 1;
    
    if(lastSelect != tempSettings.tempType)
    {
      lastSelect = tempSettings.tempType;
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Temp: %S"), (char*)pgm_read_word(&(tempType[tempSettings.tempType])));
      printCenter(tempStr, 128, 22);
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}

void settingVolt()
{
  char tempStr[20];
  uint8_t exit = 0, vLeft = 0, vRight = 0;
  float volt = tempSettings.voltWarn;
  vLeft = tempSettings.voltWarn;
  vRight = (uint8_t)(floor(tempSettings.voltWarn * 10)) % 10;
  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Volt: %d.%dv"), vLeft, vRight);
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  {
    if(checkButton(BTN_MIN) == 1)
    {
      if(tempSettings.voltWarn < 15.0)
        tempSettings.voltWarn = tempSettings.voltWarn + 0.1;
      else
        tempSettings.voltWarn = 0.0;
    }
      
    if(checkButton(BTN_HOUR) == 1)
    {
      if(tempSettings.voltWarn == 0.0)
        tempSettings.voltWarn = 15.0;
      else
        tempSettings.voltWarn = tempSettings.voltWarn - 0.1;
    }
    
    if(volt != tempSettings.voltWarn)
    {
      vLeft = tempSettings.voltWarn;
      vRight = (uint8_t)(floor(tempSettings.voltWarn * 10)) % 10;
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Volt: %d.%dv"), vLeft, vRight);
      printCenter(tempStr, 128, 22);
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}

void settingBrightness()
{
  char tempStr[20];
  uint8_t exit = 0;
  uint8_t _brightness = tempSettings.displayBrightness;
  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Bright: %d"), _brightness);
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  {
    if(checkButton(BTN_MIN) == 1)
    {
      if(tempSettings.displayBrightness < 10) tempSettings.displayBrightness += 1;
      u8g2.setContrast(5 + (tempSettings.displayBrightness * 25));
    }
      
    if(checkButton(BTN_HOUR) == 1)
    {
        if(tempSettings.displayBrightness > 0) tempSettings.displayBrightness -= 1;
        u8g2.setContrast(5 + (tempSettings.displayBrightness * 25));
    }
    
    //if(_brightness != tempSettings.displayBrightness)
    {
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Bright: %d"), _brightness);
      printCenter(tempStr, 128, 22);
      _brightness = tempSettings.displayBrightness;
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}

#ifndef BASIC
void settingRPM()
{
  char tempStr[20];
  uint8_t exit = 0, lastValue = tempSettings.rpmLimit;

  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Limit: %dRPM"), tempSettings.rpmLimit);
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  {
    if(checkButton(BTN_MIN) == 1)
      tempSettings.rpmLimit = (tempSettings.rpmLimit + 100) % 10000;
      
    if(checkButton(BTN_HOUR) == 1)
    {
      if(tempSettings.rpmLimit == 0)
        tempSettings.rpmLimit = 9900;
      else
        tempSettings.rpmLimit = (tempSettings.rpmLimit - 100) % 10000;
    }

    //if(tempSettings.rpmLimit % 100)
    //  tempSettings.rpmLimit = tempSettings.rpmLimit / 100 * 100;

    //if(tempSettings.rpmLimit > 9999) tempSettings.rpmLimit = 9999;
    
    if(lastValue != tempSettings.rpmLimit)
    {
      lastValue = tempSettings.rpmLimit;
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Limit: %dRPM"), tempSettings.rpmLimit);
      printCenter(tempStr, 128, 22);
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}

void settingSpeed()
{
  char tempStr[20];
  uint8_t exit = 0, lastValue = tempSettings.spdLimit;

  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Limit: %d%S"), tempSettings.spdLimit, (char*)pgm_read_word(&(spdType[tempSettings.speedAcr])));
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  {
    if((checkButton(BTN_MIN) == 1) && (tempSettings.spdLimit < 300))
      tempSettings.spdLimit += 5;
      
    if((checkButton(BTN_HOUR) == 1) && (tempSettings.spdLimit >= 5))
      tempSettings.spdLimit -= 5;
    
    if(lastValue != tempSettings.spdLimit)
    {
      lastValue = tempSettings.spdLimit;
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Limit: %d%S"), tempSettings.spdLimit, (char*)pgm_read_word(&(spdType[tempSettings.speedAcr])));
      printCenter(tempStr, 128, 22);
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}
#endif

void settingPane()
{
  char tempStr[20];
  uint8_t exit = 0, lastValue = tempSettings.defaultPane;

  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("Pane: %S"), (char*)pgm_read_word(&(panes[tempSettings.defaultPane])));
  printCenter(tempStr, 128, 22);
      
  while(!exit)
  {
    if(checkButton(BTN_MIN) == 1)
      tempSettings.defaultPane = (tempSettings.defaultPane + 1) % PANE_END;
      
    if(checkButton(BTN_HOUR) == 1)
    {
      if(tempSettings.defaultPane == 0)
        tempSettings.defaultPane = PANE_END - 1;
      else
        tempSettings.defaultPane = tempSettings.defaultPane - 1;
    }
    
    if(lastValue != tempSettings.defaultPane)
    {
      lastValue = tempSettings.defaultPane;
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("Pane: %S"), (char*)pgm_read_word(&(panes[tempSettings.defaultPane])));
      printCenter(tempStr, 128, 22);
    }
    
    if(checkButton(BTN_SET) == 1)
    {
      transCfg = 1;
      exit = 1;
    }
  }
}

void settingSave()
{
  memcpy(&settings, &tempSettings, sizeof(EEPROM_SETTINGS));
  EEPROM.put(EPRM_CFG_ADDR, settings);
  u8g2.clearBuffer();
  printCenter(PSTR("SAVED"), 128, 22);
  u8g2.sendBuffer();
  delay(1000);
  enterSettings = 0;
  transCfg = 0;
  settingsSelect = SETTINGS_TIME;
}

void settingQuit()
{
  u8g2.clearBuffer();
  printCenter(PSTR("CANCELLED"), 128, 22);
  u8g2.sendBuffer();
  delay(1000);
  enterSettings = 0;
  transCfg = 0;
  settingsSelect = SETTINGS_TIME;
  u8g2.setContrast(settings.displayBrightness);
}

void clearConfig()
{
  EEPROM_SETTINGS clearconfig;
  memset(&clearconfig, 0, sizeof(EEPROM_SETTINGS));
  EEPROM.put(EPRM_CFG_ADDR, clearconfig);
  u8g2.clearBuffer();
  printCenter(PSTR("CLEARED"), 128, 22);
  u8g2.sendBuffer();
  delay(1000);
}

void readConfig()
{
  bool configInvalid = false;
  EEPROM.get(EPRM_CFG_ADDR, settings);
  delay(10);
#ifndef BASIC  
  if(settings.speedAcr > SPD_KPH)
  {
    settings.speedAcr = SPD_MPH;
    configInvalid = true;
  }
#endif
  if(settings.tempType > TEMP_F)
  {
    settings.tempType = TEMP_C;
    configInvalid = true;
  }
  if(settings.voltWarn < 0.0 || settings.voltWarn > 15.0)
  {
    settings.voltWarn = 0.0;
    configInvalid = true;
  }
  if(settings.defaultPane >= PANE_END)
  {
    settings.defaultPane = PANE_TIME;
    configInvalid = true;
  }
  if(settings._24h > TIME_12)
  {
    settings._24h = TIME_24;
    configInvalid = true;
  }
  if(settings.displayBrightness > 10)
  {
    settings.displayBrightness = 10;
  }
#ifndef BASIC
  if(settings.rpmLimit > 9999)
  {
    settings.rpmLimit = 0;
    configInvalid = true;
  }
  if(settings.spdLimit > 300)
  {
    settings.spdLimit = 0;
    configInvalid = true;
  }
#endif
  paneSelect = settings.defaultPane;

#ifdef DEBUG
  Serial.println(F("Config Loaded:"));
#ifndef BASIC
  Serial.print_P(F("  Speed Acr: "));
  Serial.println(settings.speedAcr ,DEC);
#endif
  Serial.print(F("  Temp Type: "));
  Serial.println(settings.tempType ,DEC);
  Serial.print(F("  Voltage Warning: "));
  Serial.println(settings.voltWarn ,1);
  Serial.print(F("  Default Pane: "));
  Serial.println(settings.defaultPane ,DEC);
  Serial.print(F("  Display Brightness: "));
  Serial.println(settings.displayBrightness ,DEC);
#ifndef BASIC
  Serial.print(F("  Daylight Savings: "));
  Serial.println(settings.summerTime ,DEC);
  Serial.print(F("  RPM Limit: "));
  Serial.println(settings.rpmLimit ,DEC);
  Serial.print(F("  Speed Limit: "));
  Serial.println(settings.spdLimit ,DEC);
#endif
#endif

  if(configInvalid == true)
  {
    EEPROM.put(EPRM_CFG_ADDR, settings);
  }
}

int8_t drawLogo(const uint8_t* logo, uint16_t xPos , uint16_t yPos, uint16_t xSize, uint16_t ySize, bool invert, bool sendBuf)
{
  // Process 1 bit bitmap array
  // 0 = Success, 1 = Failure
  uint8_t workingByte = 0;
  uint16_t currPos = 0;
  if(!logo || xSize == 0 || ySize == 0)
    return 1;
  
  for(uint16_t i = 0; i < (ySize * xSize) / 8; i++)
  {
    for(uint16_t j = 0; j < 8; j++)
    {
      workingByte = ReverseBits(pgm_read_byte_near(logo));
      if(invert == true)
        workingByte = ~workingByte;
      if(workingByte & ((uint8_t)1 << j))
        u8g2.drawPixel(xPos + (currPos % xSize), (yPos + ((ySize - (currPos / xSize)))));
      currPos++;
    }
    logo++;
  }
  if(sendBuf == true)
    u8g2.sendBuffer();
  
  return 0;
}

int8_t displayMenu(const char * title, const char ** items, uint8_t numItems, int8_t* selection, uint8_t* active)
{
  char tempStr[20];
  int8_t lastSelection = *selection;
  u8g2.setFont(u8g2_font_calibration_gothic_nbp_tr);
  *active = 1;
  if(title != NULL)
  {
    u8g2.clearBuffer();
    printCenter(title, 128, 22);
    delay(2000);
    u8g2.sendBuffer();
  }

  u8g2.clearBuffer();
  sprintf_P(tempStr, PSTR("%S"), (char*)pgm_read_word(&(items[*selection % numItems])));
  printCenter(tempStr, 128, 22);
  
  while(enterSettings)
  {
    if(checkButton(BTN_SET) == 1)
    {
      return *selection;
    }
    
    if(checkButton(BTN_MIN) == 1)
    {
      if(*selection == 0) *selection = numItems - 1;
      else *selection = *selection - 1;
    }
    
    if(checkButton(BTN_HOUR) == 1)
    {
      *selection = (*selection + 1) % numItems;
    }
    
    //if(lastSelection != *selection)
    {
      u8g2.clearBuffer();
      sprintf_P(tempStr, PSTR("%S"), (char*)pgm_read_word(&(items[*selection % numItems])));
      printCenter(tempStr, 128, 22);
    }
  }
  return *selection;
}

#ifndef BASIC
void countRPM()
{
  rpmPulses++;
}

void countSPD()
{
  speedPulses++;
}

uint32_t getRPM()
{
  uint32_t RPM = (rpmPulses * (60000.0 / float(REFRESH_INT))) / 2.0;
  rpmPulses = 0;
  RPM = min(9999, RPM);
  return RPM;
}

uint32_t getSPD()
{
  if(!speedPulses) return 0;
  uint32_t SPD = ((((((speedPulses * (60000.0 / float(REFRESH_INT))) / FINAL_DRIVE) * (WHEEL_DIAMETER * 3.14))) * 60.0) / ((settings.speedAcr == SPD_MPH) ? 1621371.0 : 1000000.0));
  speedPulses = 0;
  SPD = min(300, SPD);
  return SPD;
}
#endif

bool isIce()
{
#ifdef ONEWIRETEMP
  temperature.requestTemperatures();
  return (temperature.getTempCByIndex(0) <= ICE_WARN) ? true : false;
#else
  return (sht21.getTemperature(TEMP_C) <= ICE_WARN) ? true : false;
#endif
}
