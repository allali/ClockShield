#include "RTClib.h"
#include <Buzzer.h>
#include <EEPROM.h>
#include "TTSDisplay.h"

#define DISP_CLK 7
#define DISP_DIO 8
#define BLANK 0x7f
#define DEGR 42
#define UNDERSCORE 130

#define LED_BLUE 2
#define LED_GREEN 3
#define LED_RED1 4
#define LED_RED2 5

#define BUZZER 6

#define BUTTON_1 11
#define BUTTON_2 10
#define BUTTON_3 9

#define TEMP A0
#define LIGHT A1

#define CHR_MINUS 16
#define CHR_C 12
#define CHR_BLANK 17

#define ALARM_HOUR sizeof(__TIME__ __DATE__) + 1
#define ALARM_MINUTE sizeof(__TIME__ __DATE__) + 2


RTC_DS1307 rtc;
//TM1636 tm1636(DISP_CLK, DISP_DIO);
TTSDisplay tm1636;
Buzzer buzzer(BUZZER);

char leds[] = { LED_BLUE, LED_GREEN, LED_RED1, LED_RED2 };
char current_led = 0;

int8_t TimeDisp[] = { 0, 0, 0, 0 };
bool DispPoint = POINT_OFF;
unsigned long lastDisp = 0;
unsigned long dispDtMs = 500; 


/* Global Datas updated on each loop call */
DateTime now;  // time from rtc module
float temperature = 0;
int lightValue = 0;
bool B_PRESSED[3] = { false, false, false };       // button was pressed less than 3sec
bool B_LONG_PRESSED[3] = { false, false, false };  // button was pressed more than 3sec
bool B_PRESS[3] = { false, false, false };         // button is currently pressed
bool alarmOn = false;
unsigned long stateStartTimeMs = 0;
char editingMode = 0;  // 0 is time ; 1 is alarm

bool afterFlashSetup = false;
bool currentAlarmClicked = false;

void setup() {
  // put your setup code here, to run once:
  buzzer.sound(NOTE_A4, 100);

  Serial.begin(57600);  // for debugging
  delay(2000);          // strange things: setup is stop and re-run after 1s,
  // possibly rtc cause a reset during init
  Serial.println("setup begin...");

  char checkVal[] = __TIME__ __DATE__;
  int j = 0;
  while (j < sizeof(checkVal) && (EEPROM.read(j) == (byte)checkVal[j])) {
    ++j;
  }

  if (j < sizeof(checkVal)) {
    afterFlashSetup = true;
    for (j = 0; j < sizeof(checkVal); ++j) EEPROM.write(j, (byte)checkVal[j]);
    Serial.println("FIRST SETUP AFTER FLASHING!!!");
    Serial.println(__DATE__);
    Serial.println(__TIME__);
    Serial.println(afterFlashSetup);
  }


  /* code to setup the real time clock module */
  if (!rtc.begin()) {  // this can loop forever if rtc module is down!!!
    Serial.flush();
    while (1) delay(10);
  }

  if ((afterFlashSetup == true) || (!rtc.isrunning())) {  // set initial date to now!
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    EEPROM.write(ALARM_HOUR, 12);
    EEPROM.write(ALARM_MINUTE, 0);
    Serial.print("RTC Time adjusted...");
    Serial.println(rtc.now().toString("DDD, DD MMM YYYY hh:mm:ss"));
  }

  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);

  pinMode(TEMP, INPUT);
  pinMode(LIGHT, INPUT);

  /* code for display */
  //tm1636.set();
  //tm1636.init();
  //tm1636.point(POINT_ON);
  //tm1636.display(TimeDisp);
  tm1636.clear();
  lastDisp = millis();

  Serial.println("setup done!");
  buzzer.sound(NOTE_A5, 100);
  delay(100);
  stateStartTimeMs = millis();
}

static void updateTemperature() {
  float resistance;
  int a;
  a = analogRead(TEMP);
  resistance = (float)(1023 - a) * 10000 / a;  //Calculate the resistance of the thermistor
  int B = 3975;
  /*Calculate the temperature according to the following formula.*/
  temperature = 1 / (log(resistance / 10000) / B + 1 / 298.15) - 273.15;
}

static void updateLightValue() {
  lightValue = analogRead(LIGHT);
}

static void updateButtonReads() {
  static int lastBRead[3] = { HIGH, HIGH, HIGH };
  static long lastBTime[3];
  static const int b[3] = { BUTTON_1, BUTTON_2, BUTTON_3 };
  static bool LONG_PRESSED_ONCE[3] = { false, false, false };

  for (int i = 0; i < 3; ++i) {
    B_PRESSED[i] = false;       // button was pressed less than 3sec
    B_LONG_PRESSED[i] = false;  // button was pressed more than 3sec
    B_PRESS[i] = false;
  }
  // LOW means button pressed, HIGH means button released
  for (int i = 0; i < 3; ++i) {
    if (digitalRead(b[i]) == LOW) {  // press
      if (lastBRead[i] == HIGH) {
        lastBTime[i] = millis();
      } else {
        if ((millis() - lastBTime[i]) > 5)  // avoid bouncing
          B_PRESS[i] = true;
        if ((millis() - lastBTime[i]) > 2500) {
          if (LONG_PRESSED_ONCE[i] == false)
            B_LONG_PRESSED[i] = true;  // TODO: MUST OCCUR ONLY ONCE!
          else
            B_LONG_PRESSED[i] = false;  // TODO: MUST OCCUR ONLY ONCE!
          LONG_PRESSED_ONCE[i] = true;
        }
      }
      lastBRead[i] = LOW;
    } else {  // release
      if (((lastBRead[i] == LOW) && (millis() - lastBTime[i]) > 5) && (LONG_PRESSED_ONCE[i] == false))
        B_PRESSED[i] = true;
      lastBRead[i] = HIGH;
      LONG_PRESSED_ONCE[i] = false;
    }
  }
}

static void printGlobals() {
  /* Debug print */
  Serial.print("temp:");
  Serial.print(temperature);

  Serial.print(" | light :");
  Serial.print(lightValue);
  for (int i = 0; i < 3; ++i) {
    Serial.print(" | B[");
    Serial.print(i);
    Serial.print("] ");
    Serial.print(B_PRESS[i]);
    Serial.print(" ");
    Serial.print(B_PRESSED[i]);
    Serial.print(" ");
    Serial.print(B_LONG_PRESSED[i]);
  }
  Serial.print(" | time: ");
  Serial.print(rtc.now().toString("DDD, DD MMM YYYY hh:mm:ss"));
  Serial.println();
}


enum StateTransition {
  ST_NONE,
  ST_MS_EDIT,
  ST_MS_PRINT_ALARM,
  ST_MS_PRINT_TEMP,
  ST_PA_DONE,
  ST_PT_DONE,
  ST_EH_DONE,
  ST_EH_GIVE_UP,
  ST_EM_GIVE_UP,
  ST_EM_DONE,
};

byte mainState() {
  //Serial.println("main state");
  if (B_LONG_PRESSED[0]) {
    editingMode = 0;
    return ST_MS_EDIT;
  }
  if (B_LONG_PRESSED[1]) {
    editingMode = 1;
    return ST_MS_EDIT;
  }
  if (B_LONG_PRESSED[2]) {  // Toggle Alarm
    alarmOn = !alarmOn;
  }
  if (B_PRESSED[2]) {
    return ST_MS_PRINT_ALARM;
  }
  if (B_PRESSED[1]) {
    return ST_MS_PRINT_TEMP;
  }
  // print time
  if (now.second() % 2)
    DispPoint = POINT_ON;
  else
    DispPoint = POINT_OFF;

  TimeDisp[0] = now.hour() / 10;
  TimeDisp[1] = now.hour() % 10;
  TimeDisp[2] = now.minute() / 10;
  TimeDisp[3] = now.minute() % 10;
  if (alarmOn)
    digitalWrite(LED_BLUE, 255);
  else
    digitalWrite(LED_BLUE, 0);

  // Check if time equals to alarm
  if ((alarmOn) && (EEPROM.read(ALARM_HOUR) == now.hour()) && (EEPROM.read(ALARM_MINUTE) == now.minute())) {
    if ((now.second() % 2) && (currentAlarmClicked == false)) {
      buzzer.sound(NOTE_A5, 100);
      buzzer.sound(NOTE_B5, 100);
    }
    for (int i = 0; i < 3; ++i)
      if (B_PRESSED[i]) currentAlarmClicked = true;
  } else {
    currentAlarmClicked = false;
  }
  //tm1636.clearDisplay();
  //tm1636.display(TimeDisp);
  return ST_NONE;
}


byte printAlarmState() {
  //Serial.println("print alarm state");
  byte h = EEPROM.read(ALARM_HOUR);
  byte m = EEPROM.read(ALARM_MINUTE);
  TimeDisp[0] = h / 10;
  TimeDisp[1] = h % 10;
  TimeDisp[2] = m / 10;
  TimeDisp[3] = m % 10;
  //tm1636.point(POINT_ON);
  DispPoint = POINT_ON;
  //tm1636.display(TimeDisp);
  if (alarmOn)
    digitalWrite(LED_BLUE, 255);
  else
    digitalWrite(LED_BLUE, 0);
  //tm1636.display(TimeDisp);

  if ((millis() - stateStartTimeMs) > 2000)  // leave state after 2sec
    return ST_PA_DONE;
  return ST_NONE;
}

byte printTempState() {
  //Serial.println("print temp state");
  static bool firstCall = true;
  if (firstCall) {
    TimeDisp[0] = ((int)temperature) / 10;
    TimeDisp[1] = ((int)temperature) % 10;
    TimeDisp[2] = ((int)temperature) % 10;
    TimeDisp[3] = CHR_C;
    DispPoint = POINT_ON;
//    tm1636.point(POINT_ON);
//    tm1636.clearDisplay();
//    tm1636.display(TimeDisp);
    digitalWrite(LED_GREEN, 255);
    firstCall = false;
  }
  if ((millis() - stateStartTimeMs) > 2000) {  // leave state after 2sec
    firstCall = true;                          // for next call
    digitalWrite(LED_GREEN, 0);
    return ST_PT_DONE;
  }
  return ST_NONE;
}

byte editHourState() {
  //Serial.println("edit hour state");
  digitalWrite(LED_RED2, HIGH);
  byte h;
  byte m;
  int dtSec = 0;
  bool changed = false;

  if (editingMode == 0) {
    h = now.hour();
    m = now.minute();
  } else {
    h = EEPROM.read(ALARM_HOUR);
    m = EEPROM.read(ALARM_MINUTE);
  }

  if (B_PRESSED[0]) {
    digitalWrite(LED_RED2, LOW);
    return ST_EH_DONE;
  }
  if (B_PRESSED[1]) {
    dtSec -= 60 * 60;
    changed = true;
  }
  if (B_PRESSED[2]) {
    dtSec += 60 * 60;
    changed = true;
  }

  //tm1636.point(POINT_ON);
  DispPoint = POINT_ON;

  TimeDisp[0] = CHR_BLANK;
  TimeDisp[1] = CHR_BLANK;
  if ((millis() / 500) % 2) {
    TimeDisp[0] = h / 10;
    TimeDisp[1] = h % 10;
  }
  TimeDisp[2] = m / 10;
  TimeDisp[3] = m % 10;

  //tm1636.display(TimeDisp);

  if (changed) {
    // record and update stateStartTimeMs
    if (editingMode == 0) {
      rtc.adjust(rtc.now() + TimeSpan(dtSec));
    } else {
      h = h + dtSec / (60 * 60);
      if (h < 0) h = 23;
      if (h > 24) h = 0;
      EEPROM.write(ALARM_HOUR, h);
    }
    stateStartTimeMs = millis();
  }
  if ((millis() - stateStartTimeMs) > 5000) {
    digitalWrite(LED_RED2, 0);
    return ST_EH_GIVE_UP;
  }
  return ST_NONE;
}

byte editMinuteState(void) {
  //Serial.println("edit minute state");
  digitalWrite(LED_RED1, HIGH);
  byte h;
  byte m;
  int dtSec = 0;
  bool changed = false;

  if (editingMode == 0) {
    h = now.hour();
    m = now.minute();
  } else {
    h = EEPROM.read(ALARM_HOUR);
    m = EEPROM.read(ALARM_MINUTE);
  }

  if (B_PRESSED[0]) {
    digitalWrite(LED_RED1, LOW);
    return ST_EM_DONE;
  }
  if (B_PRESSED[1]) {
    dtSec -= 60;
    changed = true;
  }
  if (B_PRESSED[2]) {
    dtSec += 60;
    changed = true;
  }

  //tm1636.point(POINT_ON);
  DispPoint = POINT_ON;

  TimeDisp[0] = CHR_BLANK;
  TimeDisp[1] = CHR_BLANK;
  TimeDisp[2] = CHR_BLANK;
  TimeDisp[3] = CHR_BLANK;

  TimeDisp[0] = h / 10;
  TimeDisp[1] = h % 10;

  if ((millis() / 500) % 2) {
    TimeDisp[2] = m / 10;
    TimeDisp[3] = m % 10;
  }
  //tm1636.display(TimeDisp);

  if (changed) {
    // record and update stateStartTimeMs
    if (editingMode == 0) {
      rtc.adjust(rtc.now() + TimeSpan(dtSec));
    } else {
      m = m + dtSec / 60;
      if (m < 0) m = 59;
      if (m > 60) m = 0;
      EEPROM.write(ALARM_MINUTE, m);
    }
    stateStartTimeMs = millis();
  }
  if ((millis() - stateStartTimeMs) > 5000) {
    digitalWrite(LED_RED1, 0);
    return ST_EM_GIVE_UP;
  }
  return ST_NONE;
}


byte (*currentState)(void) = mainState;

void loop() {

  /* update globals */
  updateTemperature();
  updateLightValue();
  updateButtonReads();
  now = rtc.now();  // get time for RTC module
  //printGlobals();


  byte v = currentState();
  byte (*nState)(void) = currentState;
  switch (v) {
    case ST_PA_DONE:
      nState = mainState;
      break;
    case ST_PT_DONE:
      nState = mainState;
      break;
    case ST_MS_PRINT_ALARM:
      nState = printAlarmState;
      break;
    case ST_MS_PRINT_TEMP:
      nState = printTempState;
      break;
    case ST_MS_EDIT:
      nState = editHourState;
      break;
    case ST_EH_DONE:
      nState = editMinuteState;
      break;
    case ST_EH_GIVE_UP:
      nState = mainState;
      break;
    case ST_EM_DONE:
      nState = mainState;
      break;
    case ST_EM_GIVE_UP:
      nState = mainState;
      break;
  }
  if (nState != currentState) {
    stateStartTimeMs = millis();
    currentState = nState;
  }

  if ((millis() - lastDisp)>dispDtMs){
    Serial.print("Display: ");
    Serial.print(TimeDisp[0]);
    Serial.print(TimeDisp[1]);
    if (DispPoint==POINT_ON)
      Serial.print(":");
      else
      Serial.print(" ");
    Serial.print(TimeDisp[2]);
    Serial.print(TimeDisp[3]);
    uchar b=map(lightValue,350,900,BRIGHT_DARKEST,BRIGHTEST);
    Serial.print(" bright: ");
    Serial.print(lightValue);
    Serial.print(" => ");
    Serial.println(b);

    tm1636.brightness(b);
    tm1636.clear();
    for(int i=0;i<4;++i)
      tm1636.display(3-i, TimeDisp[i]);
    if (DispPoint==POINT_ON)
      tm1636.pointOn();
    else
      tm1636.pointOff();

    
    //tm1636.clearDisplay();
    //tm1636.point(DispPoint);
    //tm1636.display(TimeDisp);

    lastDisp=millis();
  }

  delay(10);


  /*
  TimeDisp[0] = BLANK;
  TimeDisp[1] = BLANK;
  TimeDisp[2] = BLANK;

  if (digitalRead(BUTTON_1) == LOW) {
    TimeDisp[3] = 1;
  } else if (digitalRead(BUTTON_2) == LOW) {
    TimeDisp[3] = 2;
  } else if (digitalRead(BUTTON_3) == LOW) {
    TimeDisp[3] = 3;
  } else {
    if (now.second() % 2)
      tm1636.point(POINT_ON);
    else
      tm1636.point(POINT_OFF);
    TimeDisp[0] = now.hour() / 10;
    TimeDisp[1] = now.hour() % 10;
    TimeDisp[2] = now.minute() / 10;
    TimeDisp[3] = now.minute() % 10;
  }
  tm1636.display(TimeDisp);

  for (int i = 0; i < sizeof(leds); ++i) {
    if (i == current_led)
      digitalWrite(leds[i], 255);
    else
      digitalWrite(leds[i], 0);
  }
  current_led = (current_led + 1) % sizeof(leds);

*/
  //delay(1000);
}
