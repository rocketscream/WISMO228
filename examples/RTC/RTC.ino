/*******************************************************************************
* WISMO228 Library - RTC Usage Example
* Version: 1.00
* Date: 28-03-2012
* Company: Rocket Scream Electronics
* Website: www.rocketscream.com
*
* This is an example on RTC usage with the WISMO228 library on the TraLog 
* Shield.
*
* ============
* Requirements
* ============
* 1. UART selection switch to SW position (uses pin D5 (RX) & D6 (TX)).
* 2. Jumper J14 is closed to allow usage of pin A2 to control on-off state of 
*    WISMO228 module.
* 3. If you plan to maintain the RTC even with main power removed, insert a
*    CR1220 Lithium 3V coin cell in the coin cell holder on the bottom side of 
*    the TraLog Shield.It should last you for nearly 12 months!
*
* This example is licensed under Creative Commons Attribution-ShareAlike 3.0 
* Unported License. 
*
* Revision  Description
* ========  ===========
* 1.00      Initial public release. Tested with L22 & L23 of WISMO228 firmware.
*           Only works with Arduino IDE 1.0.
*******************************************************************************/
// ***** INCLUDES *****
#include "SoftwareSerial.h"
#include <WISMO228.h>

// ***** PIN ASSIGNMENT *****
const  uint8_t  rxPin = 5;
const  uint8_t  txPin = 6;
const  uint8_t  onOffPin = A2;

// ***** CONSTANTS *****
// Clock in YY/MM/DD,HH:MM:SS*ZZ format
// ZZ represent time difference between local & GMT time expressed in quarters
// of an hour. *ZZ has a range of -48 to +48. 
// Example: 12/03/28,12:12:00+32 (GMT+8 local time).
char  newClock[CLOCK_COUNT_MAX+1] = "12/03/28,12:12:00+32";

// ***** VARIABLES *****
char  clock[CLOCK_COUNT_MAX+1];

// WISMO228 class
WISMO228  wismo(rxPin, txPin, onOffPin);

void setup()  
{
  Serial.begin(9600);
  Serial.println("RTC Example");

  // Initialize WISMO228
  wismo.init();

  Serial.println("Powering up, please wait...");

  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    Serial.println("TraLog is awake!");
    Serial.println("Let's check the RTC!");

    // Retrieve current time & date
    if (wismo.getClock(clock))
    {
      Serial.print("Current clock: ");
      Serial.println(clock);

      if (wismo.setClock(newClock))
      {
        Serial.println("New time & date set. Let's check it out.");
      }
    }
  }
  else
  {
    Serial.println("Ugh, power up failed.");
  }
}

void loop()
{
  // Let's check the new time & date
  if (wismo.getClock(clock))
  {
    Serial.print("Current clock: ");
    Serial.println(clock);
  }
  // Check RTC every 1 s
  delay(1000);
}