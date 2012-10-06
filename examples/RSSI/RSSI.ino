/*******************************************************************************
* WISMO228 Library - GSM RSSI Example
* Version: 1.00
* Date: 28-03-2012
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an example on retrieving RSSI (Receive Signal Strength Indicator) of
* the GSM signal using the WISMO228 library on the TraLog Shield. Retrieved 
* RSSI value is in units of dBm. 
*
* ============
* Requirements
* ============
* 1. UART selection switch to SW position (uses pin D5 (RX) & D6 (TX)).
* 2. Jumper J14 is closed to allow usage of pin A2 to control on-off state of 
*    WISMO228 module.
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

// ***** VARIABLES *****
bool	status;

// WISMO228 class
WISMO228  wismo(rxPin, txPin, onOffPin);

void setup()  
{
  // Initialize WISMO228 module start up status
  status = false;

  Serial.begin(9600);
  Serial.println("GSM RSSI Example");

  // Initialize WISMO228
  wismo.init();
  
  Serial.println("Powering up, please wait...");
  
  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    // Power up is successful
    status = true;
    Serial.println("TraLog is awake!");
  }
  else
  {
    Serial.println("Ugh, power up failed.");
  }
}

void loop()                 
{
  // If WISMO228 module is power up properly
  if (status)
  {
    Serial.print("RSSI: ");
    Serial.print(wismo.getRssi(), DEC);
    Serial.println(" dBm");
  }
  // Query RSSI every 5 s
  delay(5000);
}