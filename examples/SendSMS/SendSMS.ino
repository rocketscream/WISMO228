/*******************************************************************************
* WISMO228 Library - Send SMS Example
* Version: 1.00
* Date: 28-03-2012
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an example on sending SMS using the WISMO228 library on the TraLog 
* Shield.
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
*           Only works with Arduino IDE 1.0 & 1.0.1.
*******************************************************************************/
// ***** INCLUDES *****
#include "SoftwareSerial.h"
#include <WISMO228.h>

// ***** PIN ASSIGNMENT *****
const  uint8_t  rxPin = 5;
const  uint8_t  txPin = 6;
const  uint8_t  onOffPin = A2;

// ***** CONSTANTS *****
const  char  phoneNumber[] = "+1234567890";
const  char  message[] = "Hello Hello...";

// WISMO228 class
WISMO228  wismo(rxPin, txPin, onOffPin);

void setup()  
{
  Serial.begin(9600);
  Serial.println("Sending SMS Example");

  // Initialize WISMO228
  wismo.init();
  
  Serial.println("Powering up, please wait...");
  
  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    Serial.println("TraLog is awake!");
    Serial.println("Let's send some SMS!");
    Serial.println("Sending SMS, please wait...");
    
    if (wismo.sendSms(phoneNumber, message))
    {
      Serial.println("Sending SMS complete.");
    }
    else
    {
      Serial.println("Sending SMS failed.");
    }
  }
  else
  {
    Serial.println("Ugh, power up failed.");
  }
}

void loop()                 
{
  // Let's count sheep instead
}