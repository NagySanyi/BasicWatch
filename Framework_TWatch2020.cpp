// Framework_TWatch2020.cpp
// Part of  BasicWatch - ttgo t-watch-2020
// 
// Based on Arduino based framework for the Lilygo T-Watch 2020
// Much of the code is based on the sample apps for the
// T-watch that were written and copyrighted by Lewis He.
//(Copyright (c) 2019 lewis he)
// https://www.instructables.com/id/Lilygo-T-Watch-2020-Arduino-Framework/
// By DanGeiger


#include "config.h"
#include <time.h>
#include <soc/rtc.h>

TTGOClass *ttgo;
TFT_eSPI *tft;
BMA *sensor;
RTC_Date tnow;
uint8_t hh, mm, ss, mmonth, dday;     // H, M, S variables
uint16_t yyear;         // Year is 16 bit int
int sleepTime;
bool sleeping = false;
int16_t x, y;



#define uS_TO_S_FACTOR 1000000

bool irq = false;
uint32_t tiltCount = 0;
uint32_t clickCount = 0;
uint32_t stepCount = 0;
int per;
int sleepCountdown;

int16_t xoffset = 30;


void Basic(void);
int getTnum();
void prtTime(byte digit);
void setMenuDisplay(int mSel);

byte xcolon = 0; // location of the colon

// Show the accelerometer working

void appAccel()
{
    ttgo->bma->begin();
    ttgo->bma->enableAccel();
    ttgo->tft->fillScreen(TFT_BLACK);
    int16_t x, y;
    int16_t xpos, ypos;

    Accel acc;

    while (!ttgo->getTouch(x, y)) { // Wait for touch to exit

        ttgo->bma->getAccel(acc);
        xpos = acc.x;
        ypos = acc.y;
        ttgo->tft->fillCircle(xpos / 10 + 119, ypos / 10 + 119, 10, TFT_RED); // draw dot
        delay(100);
        ttgo->tft->fillCircle(xpos / 10 + 119, ypos / 10 + 119, 10, TFT_BLACK); // erase previous dot
    }

    while (ttgo->getTouch(x, y)) {}  // Wait for release to return to the clock

    ttgo->tft->fillScreen(TFT_BLACK); // Clear screen
}


// Check out the touch screen
// Will display the x and y coordinates of where you touch
// for 10 seconds and then return to time

void appTouch()
{
    uint32_t endTime = millis() + 10000; // Timeout at 10 seconds
    int16_t x, y;
    ttgo->tft->fillScreen(TFT_BLACK);

    while (endTime > millis()) {
        ttgo->getTouch(x, y);
        ttgo->tft->fillRect(98, 100, 70, 85, TFT_BLACK);
        ttgo->tft->setCursor(80, 100);
        ttgo->tft->print("X:");
        ttgo->tft->println(x);
        ttgo->tft->setCursor(80, 130);
        ttgo->tft->print("Y:");
        ttgo->tft->println(y);
        delay(25);
    }

    while (ttgo->getTouch(x, y)) {}  // Wait for release to exit
    ttgo->tft->fillScreen(TFT_BLACK);
}

// Set the time - no error checking, you might want to add it
// Set the time - no error checking, you might want to add it

void appSetTime() {
  // Get the current info
  RTC_Date tnow = ttgo->rtc->getDateTime();

  hh = tnow.hour;
  mm = tnow.minute;
  ss = tnow.second;
  dday = tnow.day;
  mmonth = tnow.month;
  yyear = tnow.year;

  //Set up the interface buttons

  ttgo->tft->fillScreen(TFT_BLACK);
  ttgo->tft->fillRect(0, 35, 80, 50, TFT_BLUE);
  ttgo->tft->fillRect(161, 35, 78, 50, TFT_BLUE);
  ttgo->tft->fillRect(81, 85, 80, 50, TFT_BLUE);
  ttgo->tft->fillRect(0, 135, 80, 50, TFT_BLUE);
  ttgo->tft->fillRect(161, 135, 78, 50, TFT_BLUE);
  ttgo->tft->fillRect(0, 185, 80, 50, TFT_BLUE);
  ttgo->tft->setTextColor(TFT_GREEN);
  ttgo->tft->drawNumber(1, 30, 40, 2);
  ttgo->tft->drawNumber(2, 110, 40, 2);
  ttgo->tft->drawNumber(3, 190, 40, 2);
  ttgo->tft->drawNumber(4, 30, 90, 2);
  ttgo->tft->drawNumber(5, 110, 90, 2);
  ttgo->tft->drawNumber(6, 190, 90, 2);
  ttgo->tft->drawNumber(7, 30, 140, 2);
  ttgo->tft->drawNumber(8, 110, 140, 2);
  ttgo->tft->drawNumber(9, 190, 140, 2);
  ttgo->tft->drawNumber(0, 30, 190, 2);
  ttgo->tft->fillRoundRect(120, 200, 119, 39, 6, TFT_WHITE);
  ttgo->tft->setTextSize(2);
  ttgo->tft->setCursor(0, 0);

  ttgo->tft->setCursor(155, 210);
  ttgo->tft->setTextColor(TFT_BLACK);
  ttgo->tft->print("DONE");
  ttgo->tft->setTextColor(TFT_WHITE);
  int wl = 0; // Track the current number selected
  byte curnum = 1;  // Track which digit we are on

  prtTime(curnum); // Display the time for the current digit

  while (wl != 13) {
    wl = getTnum();
    if (wl != -1) {

      switch (curnum) {
        case 1:
          hh = wl * 10 + hh % 10;
          break;
        case 2:
          hh = int(hh / 10) * 10 + wl;
          break;
        case 3:
          mm = wl * 10 + mm % 10;
          break;
        case 4:
          mm = int(mm / 10) * 10 + wl;
          break;
      }
      while (getTnum() != -1) {}
      curnum += 1;
      if (curnum > 4) curnum = 1;
      prtTime(curnum);
    }
  }
  while (getTnum() != -1) {}
  ttgo->rtc->setDateTime(yyear, mmonth, dday, hh, mm, 0);
  ttgo->tft->fillScreen(TFT_BLACK);
}

// prtTime will display the current selected time and highlight
// the current digit to be updated in yellow

void prtTime(byte digit) {
  ttgo->tft->fillRect(0, 0, 100, 34, TFT_BLACK);
  if (digit == 1)   ttgo->tft->setTextColor(TFT_YELLOW);
  else   ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->drawNumber(int(hh / 10), 5, 5, 2);
  if (digit == 2)   ttgo->tft->setTextColor(TFT_YELLOW);
  else   ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->drawNumber(hh % 10, 25, 5, 2);
  ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->drawString(":",  45, 5, 2);
  if (digit == 3)   ttgo->tft->setTextColor(TFT_YELLOW);
  else   ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->drawNumber(int(mm / 10), 65 , 5, 2);
  if (digit == 4)   ttgo->tft->setTextColor(TFT_YELLOW);
  else   ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->drawNumber(mm % 10, 85, 5, 2);
}

// getTnum takes care of translating where we pressed into
// a number that was pressed. Returns -1 for no press
// and 13 for DONE

int getTnum() {
  int16_t x, y;
  if (!ttgo->getTouch(x, y)) return - 1;
  if (y < 85) {
    if (x < 80) return 1;
    else if (x > 160) return 3;
    else return 2;
  }
  else if (y < 135) {
    if (x < 80) return 4;
    else if (x > 160) return 6;
    else return 5;
  }
  else if (y < 185) {
    if (x < 80) return 7;
    else if (x > 160) return 9;
    else return 8;
  }
  else if (x < 80) return 0;
  else return 13;
}


// Display Battery Data

void appBattery()
{
    ttgo->tft->fillScreen(TFT_BLACK);
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    ttgo->tft->drawString("BATT STATS",  35, 30, 2);
    ttgo->tft->setTextColor(TFT_GREEN, TFT_BLACK);

    // Turn on the battery adc to read the values
    ttgo->power->adc1Enable(AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, true);
    // get the values
    float vbus_v = ttgo->power->getVbusVoltage();
    float vbus_c = ttgo->power->getVbusCurrent();
    float batt_v = ttgo->power->getBattVoltage();
    int per = ttgo->power->getBattPercentage();

// Print the values
    ttgo->tft->setCursor(0, 100);
    ttgo->tft->print("Vbus: "); ttgo->tft->print(vbus_v); ttgo->tft->println(" mV");
    ttgo->tft->setCursor(0, 130);
    ttgo->tft->print("Vbus: "); ttgo->tft->print(vbus_c); ttgo->tft->println(" mA");
    ttgo->tft->setCursor(0, 160);
    ttgo->tft->print("BATT: "); ttgo->tft->print(batt_v); ttgo->tft->println(" mV");
    ttgo->tft->setCursor(0, 190);
    ttgo->tft->print("Per: "); ttgo->tft->print(per); ttgo->tft->println(" %");

    int16_t x, y;
    while (!ttgo->getTouch(x, y)) {} // Wait for touch
    while (ttgo->getTouch(x, y)) {}  // Wait for release to exit
    //Clear screen
    ttgo->tft->fillScreen(TFT_BLACK);
}


void watchSleepyTime() {
  setCpuFrequencyMhz(20);
 ttgo->closeBL();
 ttgo->displaySleep();
  sleeping = true;
  esp_light_sleep_start();
}



// The basic menu code
// If you add an app, this is where you will update the
// framework code to include it in the menu.
//
// Make the following updates:
// 1) Update maxApp to the total number of apps.
// 2) Update appName to add the title of the app.
// 3) in the main routine in TWatch_framework, add a case to the switch statement to call your app routine.


const int maxApp = 6; // number of apps
String appName[maxApp] = {"Clock", "Basic", "Accel", "Battery", "Touch", "Set Time"}; // app names

uint8_t modeMenu() {
  sleepTime = millis() + 8000;
  int mSelect = 0; // The currently highlighted app
  int16_t x, y, tx, ty;

  boolean exitMenu = false; // used to stay in the menu until user selects app

  setMenuDisplay(0); // display the list of Apps

  while (!exitMenu) {
    if (ttgo->getTouch(x, y)) { // If you have touched something...

      while (ttgo->getTouch(tx, ty)) {} // wait until you stop touching
      //you touched the menu, give 8 seconds to wait.
      sleepTime = millis() + 8000;
      
      if (y >= 160) { // you want the menu list shifted up
        mSelect += 1;
        if (mSelect == maxApp) mSelect = 0;
        setMenuDisplay(mSelect);
      }

      if (y <= 80) { // you want the menu list shifted down
        mSelect -= 1;
        if (mSelect < 0) mSelect = maxApp - 1;
        setMenuDisplay(mSelect);
      }
      if (y > 80 && y < 160) { // You selected the middle
        exitMenu = true;
      }
    }
  if (millis() > sleepTime) {
    watchSleepyTime();
    }
  }
  //Return with mSelect containing the desired mode
 ttgo->tft->fillScreen(TFT_BLACK);
  return mSelect;
}

void setMenuDisplay(int mSel) {
 ttgo->tft->setTextFont(1);
  int curSel = 0;
  // Display mode header
 ttgo->tft->fillScreen(TFT_BLUE);
 ttgo->tft->fillRect(0, 80, 239, 80, TFT_BLACK);

  // Display apps
  if (mSel == 0) curSel = maxApp - 1;
  else curSel = mSel - 1;

 ttgo->tft->setTextSize(2);
 ttgo->tft->setTextColor(TFT_GREEN);
 ttgo->tft->setCursor(50, 30);
 ttgo->tft->println(appName[curSel]);

 ttgo->tft->setTextSize(3);
 ttgo->tft->setTextColor(TFT_RED);
 ttgo->tft->setCursor(40, 110);
 ttgo->tft->println(appName[mSel]);

  if (mSel == maxApp - 1) curSel = 0;
  else curSel = mSel + 1;

 ttgo->tft->setTextSize(2);
 ttgo->tft->setTextColor(TFT_GREEN);
 ttgo->tft->setCursor(50, 190);
 ttgo->tft->print(appName[curSel]);
}


void handleMenu() {
     switch (modeMenu()) { // Call modeMenu. The return is the desired app number
        case 0: // Zero is the clock, just exit the switch
            break;
        case 1:
            Basic();
            break;
        case 2:
            appAccel();
            break;
        case 3:
            appBattery();
            break;
        case 4:
            appTouch();
            break;
        case 5:
            appSetTime();
            break;
        }
}

//

void timer_wakeup() {
  Serial.println("You woke up from a timer.  This will fire every 60 seconds to do things like alarms.");
  digitalWrite(4, HIGH);
  delay(300);
  digitalWrite(4, LOW);
  sleeping = true;
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 1);
  esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * 15 * 60);
  esp_light_sleep_start();
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : timer_wakeup(); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void wakeIfNecessary() {
  if (sleeping == true) {
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
      timer_wakeup();
    } else {
     ttgo->displayWakeup();
     ttgo->openBL();
      sleeping = false;
      sleepTime = millis() + 5000;
     ttgo->rtc->syncToSystem();
      setCpuFrequencyMhz(160);
    }
  }
}

void homeScreen() {
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
 tft->setTextFont(4);
    if (irq) {
        irq = 0;
        bool  rlst;
        do {
            // Read the BMA423 interrupt status,
            // need to wait for it to return to true before continuing
            rlst =  sensor->readInterrupt();
        } while (!rlst);
    }

    if (sensor->getCounter() != stepCount) {
      tft->setTextColor(TFT_BLACK);
    tft->setCursor(xoffset, 42);
    tft->print("StepCount:");
    tft->print(stepCount);
    }
    stepCount = sensor->getCounter();
    tft->setTextColor(TFT_RED);
    tft->setCursor(xoffset, 42);
    tft->print("StepCount:");
    tft->print(stepCount);

    if ((ttgo->power->getBattPercentage()) != per) {
      tft->setTextColor(TFT_BLACK);
      tft->setCursor(xoffset, 84);
      tft->print("Battery:");
      tft->print(per);
    }
    per =ttgo->power->getBattPercentage();
    tft->setTextColor(TFT_GREEN);
    tft->setCursor(xoffset, 84);
    tft->print("Battery:");
    tft->print(per);

    if (ttgo->rtc->getDateTime().minute != mm) {
      tft->setTextColor(TFT_BLACK);
      tft->setTextFont(7);
      tft->setCursor(xoffset, 126);

      tft->print(hh);
      tft->print(":");
      if (mm < 10) {
        tft->print("0");
      }
      tft->print(mm);
    }

    tnow =ttgo->rtc->getDateTime();
    hh = tnow.hour;
    mm = tnow.minute;
    ss = tnow.second;
    dday = tnow.day;
    mmonth = tnow.month;
    yyear = tnow.year;

    tft->setTextColor(TFT_WHITE);
    tft->setTextFont(7);
    tft->setCursor(xoffset, 126);
 
    tft->print(hh);
    tft->print(":");
    if (mm < 10) {
      tft->print("0");
    }
    tft->print(mm);
 
 tft->setTextFont(4);
 if (ttgo->getTouch(x, y)) {			// wait to get a touch.
    Serial.println("You touched me!");
    while (ttgo->getTouch(x, y)) {} 		// wait for user to release
    handleMenu();				// if you touched something, show the menu.
   tft->setTextFont(4);
   ttgo->tft->setTextSize(1);
  }


}


void watch_init()
{
    Serial.begin(115200);
    pinMode(4, OUTPUT);
    print_wakeup_reason();
    // Get TTGOClass instance
   ttgo = TTGOClass::getWatch();

   ttgo->begin();
   ttgo->rtc->check();
   ttgo->rtc->syncToSystem();
   ttgo->openBL();

    tft =ttgo->tft;
    sensor =ttgo->bma;

    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    sensor->accelConfig(cfg);

    sensor->enableAccel();	    			// Enable BMA423 accelerometer
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] {
         irq = 1;			       		// Set interrupt to set irq value to 1
    }, RISING); 					//It must be a rising edge

    sensor->enableFeature(BMA423_STEP_CNTR, true);	// Enable BMA423 isStepCounter feature
    sensor->enableFeature(BMA423_TILT, true);		// Enable BMA423 isTilt feature
    sensor->enableFeature(BMA423_WAKEUP, true);		// Enable BMA423 isDoubleClick feature

    //sensor->resetStepCounter();			// Reset steps
    sensor->enableStepCountInterrupt(false);		// Turn on feature interrupt
    sensor->enableTiltInterrupt();
    sensor->enableWakeupInterrupt();			// It corresponds to isDoubleClick interrupt

    tft->setTextFont(4);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR * 15 * 60);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_39, 1);
    sleepTime = millis() + 5000;
}
