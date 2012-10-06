/*******************************************************************************
* WISMO228 Library - Receive SMS Example
* Version: 1.00
* Date: 28-03-2012
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an example on receiving SMS using the WISMO228 library on the TraLog 
* Shield.
*
* ============
* Requirements
* ============
* 1. UART selection switch to SW position (uses pin D5 (RX) & D6 (TX)).
* 2. Jumper J14 is closed to allow usage of pin A2 to control on-off state of 
*    WISMO228 module.
* 3. Jumper J15 is closed to allow usage of pin D2 as interupt source of the RI
*    signal on WISMO228 module.
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
const  uint8_t  ringPin = 2;

// ***** VARIABLES *****
volatile bool    sms = false;

// WISMO228 class
WISMO228 wismo(rxPin, txPin, onOffPin, ringPin, newSms);

void setup()  
{
  Serial.begin(9600);
  Serial.println("Receiving SMS Example");

  // Initialize WISMO228
  wismo.init();
  
  Serial.println("Powering up, please wait...");
  
  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    Serial.println("TraLog is awake!");
    Serial.println("Try sending some SMS to TraLog!");
  }
  else
  {
    Serial.println("Ugh, power up failed.");
  }
}

void loop()                 
{
  // Sender phone number
  char  sender[12];
  // Receive message buffer (adjust size accrdingly)
  char  message[50];

  // If new SMS is received
  if (sms)
  {
    sms = false;
    Serial.println("We just got a new SMS!");
    
    // Read the new SMS
    if (wismo.readSms(sender, message))
    {
      Serial.println(sender);
      Serial.println(message);
    }
  }
}

/*******************************************************************************
* Name: newSms
* Description: A handler for new SMS indication.
*
* Argument     Description
* =========    ===========
* 1. NIL				
*
* Return       Description
* =========	   ===========
* 1. NIL			
*
*******************************************************************************/
void    newSms(void)
{
  sms = true;
}