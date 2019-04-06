#if !defined(ESP8266)
#define D2                      2
#define D3                      3
#define D4                      4
#define D5                      5
#define D6                      6
#define D7                      7
#endif

// Buttons
#define BTN_SET                 D4
#define BTN_MIN                 D5
#define BTN_HOUR                D6

// Pin used for voltage detection
#define VOLT_IN                 A0

// Pin used for RPM and Speed
#define RPM_PIN                 D2
#define SPD_PIN                 D3

// Pin used for 1 wire temperature sensor
#define TEMP_IN                 D7

// Pane Numbers 
#ifndef BASIC
#define PANE_TIME               0
#define PANE_TEMP               1
#define PANE_VOLT               2
#define PANE_RPM                3
#define PANE_SPD                4
#define PANE_END                5
#else
#define PANE_TIME               0
#define PANE_TEMP               1
#define PANE_VOLT               2
#define PANE_END                3
#endif
// Settings
#ifndef BASIC
#define SETTINGS_TIME           0
#define SETTINGS_TTYPE          1
#define SETTINGS_SPDT           2
#define SETTINGS_TEMP           3
#define SETTINGS_VOLT           4
#define SETTINGS_BRIGHTNESS     5
#define SETTINGS_RPM            6
#define SETTINGS_SPD            7
#define SETTINGS_PANE           8
#define SETTINGS_SAVE           9
#define SETTINGS_QUIT           10
#define SETTINGS_END            11
#else
#define SETTINGS_TIME           0
#define SETTINGS_TTYPE          1
#define SETTINGS_TEMP           2
#define SETTINGS_VOLT           3
#define SETTINGS_BRIGHTNESS     4
#define SETTINGS_PANE           5
#define SETTINGS_SAVE           6
#define SETTINGS_QUIT           7
#define SETTINGS_END            8
#endif

// Time required for settings button to be held
#define SETTINGS_ENTR           3

#define SETTING_TIME_TIME       0
#define SETTING_TIME_DATE       1
#define SETTING_TIME_SAVE       2
#define SETTING_TIME_BACK       3
#define SETTING_TIME_END        4
#define SETTING_DATE_DAY        0
#define SETTING_DATE_MTH        1
#define SETTING_DATE_YR         2
#define SETTING_DATE_DOW        3
#define SETTING_DATE_END        4

// Temperature to display ice warning on screen
#define ICE_WARN                3.0

// Speed Acronyms
#define SPD_MPH                 0
#define SPD_KPH                 1

// Temperature type
#define TEMP_C                  0
#define TEMP_F                  1

// Time Summer support
#define TIME_24                 0
#define TIME_12                 1

// RPM

// Speed
// Tyre size in mm - 65R14 
#define WHEEL_DIAMETER          609.0
// Final Drive - Stock 4.3:1
#define FINAL_DRIVE             4.3

// Resisters used in voltage divider
#define R1                      30000.0
#define R2                      7500.0
#define VOLTAGE_SAMPLE          10
uint8_t voltSampleCount = 0;

// Debounce
#define DEBOUNCE                500

// EEPROM
#define EPRM_CFG_ADDR           0
typedef struct eepromSettings
{
  uint8_t tempType;           // C or F temperature
  uint8_t _24h;               // 24 or 12 Hour Clock
  float voltWarn;             // Battery voltage warning value
  uint8_t displayBrightness;  // Screen Brightness
  uint8_t defaultPane;        // Default on boot
#ifndef BASIC
  uint16_t rpmLimit;   // Engine RPM Limit (when to display shift indicator)
  uint16_t spdLimit;   // Speed Limit (when to display speed warning)
  uint8_t speedAcr;    // MPH or KPH
#endif
} EEPROM_SETTINGS;
uint8_t transCfg = 0;

#define ReverseBits(in) ((in & 0x01) << 7) | ((in & 0x02) << 5) | ((in & 0x04) << 3) | ((in & 0x08) << 1) | ((in & 0x10) >> 1) | ((in & 0x20) >> 3) | ((in & 0x40) >> 5) | ((in & 0x80) >> 7)
#if defined(IS_TEENSY) || defined(IS_STM32)
#define GetADC(value) (value * 3.3) / 4096.0
#else
#define GetADC(value) (value * 5.05) / 1024.0
#endif

uint32_t timer = 0; 
uint8_t counter = 0;
int8_t paneSelect = PANE_TIME;
int8_t LEDTOGGLE = 0;
#ifndef BASIC
#define REFRESH_INT   1000
uint32_t previousMillis = 0;
volatile uint32_t rpmPulses = 0;
volatile uint32_t speedPulses = 0;
#endif 

EEPROM_SETTINGS settings;
EEPROM_SETTINGS tempSettings;

const char * const speed_acr[] _PROGMEM = {STR_ACKMPH, STR_ACKKPH}; // MPH and KPH in custom font
const char * const spdType[] _PROGMEM  = {STR_MPH, STR_KPH};
const char * const tempType[] _PROGMEM  = {STR_TEMPC, STR_TEMPF};
const char * const settingState[] _PROGMEM  = {STR_OFF, STR_ON};
const char * const clockType[] _PROGMEM  = {STR_24, STR_12};
uint32_t lastDebounceTime = 0;
#ifndef BASIC
uint16_t carSpeed = 0;
uint16_t engineRpm = 0;
#endif
float volt = 0.0;
uint32_t sampleVolts = 0;

// Menu
uint8_t settingsHold = 0;
uint8_t enterSettings = 0;
uint8_t enterTimeSettings = 0;
int8_t timeSelect = 0;
int8_t settingsSelect = SETTINGS_TIME;
const char * const strSetting[] _PROGMEM = {
  STR_SETTIME,
  STR_CLOCKTYPE,
#ifndef BASIC
  STR_SPEEDTYPE,
#endif
  STR_TEMPTYPE,
  STR_VOLTWARN,
  STR_BRIGHTNESS,
#ifndef BASIC
  STR_RPMLIMIT,
  STR_SPEEDLIMIT,
#endif
  STR_DEFAULTPANE,
  STR_SAVE,
  STR_QUIT
};

const char * const strTimeSetting[] _PROGMEM = { 
  STR_TIME,
  STR_DATE,
  STR_SAVE,
  STR_BACK
};

const char * const panes[] _PROGMEM = {
  STR_TIME,
  STR_TEMP,
  STR_VOLT,
#ifndef BASIC
  STR_RPM,
  STR_SPEED
#endif
};

const char * const days[] _PROGMEM = {
  STR_SUN,
  STR_MON,
  STR_TUE,
  STR_WED,
  STR_THUR,
  STR_FRI,
  STR_SAT
};

uint16_t workingHour;
uint16_t workingMin;
uint16_t workingDay;
uint16_t workingNDay;
uint16_t workingMonth;
uint16_t workingYear;

// Images
#ifdef IS_ARDUINO
#ifndef NO_LOGO
extern const byte _PROGMEM boot_logo[];
#endif
extern const byte _PROGMEM bat_logo[];
#endif

// Screen
#define U8G2_P        U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C
U8G2_P u8g2(U8G2_R0);

// SHT21 - Temperature Sensor
//#ifndef BASIC
#ifndef ONEWIRETEMP
SHT21 sht21;
#else
OneWire oneWireTemp(TEMP_IN);
DallasTemperature temperature(&oneWireTemp);
#endif
//#endif
