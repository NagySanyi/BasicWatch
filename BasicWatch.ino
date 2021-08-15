//  BasicWatch.ino
//  Simple WatchBasic example for a ttgo-watch2020 V1.


#include "config.h"

void watch_init();
void WatchBasic_init();
void wakeIfNecessary();
void homeScreen();
void watchSleepyTime();

extern int sleepTime;

void setup()
{
watch_init();
WatchBasic_init();
}

void loop() 
{
  wakeIfNecessary();
  homeScreen();					// Display "home" screen 
  if (millis() > sleepTime) {			// if the timer has expired, go to sleep
    watchSleepyTime();
  }
  delay(100);
}
