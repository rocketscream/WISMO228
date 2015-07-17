/*******************************************************************************
* WISMO228 Library - Ping Example
* Version: 1.10
* Date: 09-07-2013
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an example on ping using the WISMO228 library on the TraLog Shield.
*
* ============
* Requirements
* ============
* 1. UART selection switch to SW position (uses pin D5 (RX) & D6 (TX)).
* 2. On v1 of the shield, jumper J14 is closed to allow usage of pin A2 to 
*    control on-off state of WISMO228 module. On v2 of the shield, short the 
*    jumper labelled A2 & GSM-ON. This is the default factory setting.
* 3. You need to know your service provider APN name, username, and password. If
*    they don't specify the username and password, you can use " ". Notice the
*    space in between the quote mark. 
*
* This example is licensed under Creative Commons Attribution-ShareAlike 3.0 
* Unported License. 
*
* Revision  Description
* ========  ===========
* 1.10      Updated to support WISMO228 Library version 1.20.
*           Tested up to Arduino IDE 1.0.4.
* 1.00      Initial public release. Tested with L22 & L23 of WISMO228 firmware.
*           Only works with Arduino IDE 1.0.
*******************************************************************************/
// ***** INCLUDES *****
//#include "SoftwareSerial.h"
#include <WISMO228.h>

// ***** PIN ASSIGNMENT *****
const  uint8_t  gsmRxPin = 5;
const  uint8_t  gsmTxPin = 6;
const  uint8_t  gsmOnOffPin = A2;

// ***** CONSTANTS *****
const  char  apn[] = "apn";
const  char  username[] = "username";
const  char  password[] = "password";

// ***** CLASSES *****
// Software serial class
//SoftwareSerial gsm(gsmRxPin, gsmTxPin); 
// WISMO228 class
//WISMO228  wismo(&gsm, gsmOnOffPin);
WISMO_CREATE_DEFAULT_INSTANCE()

void setup()  
{
	Serial.begin(115200);
	while (!Serial) { ; } // wait for serial port to connect. Needed for Leonardo only
	Serial.println("Ping Example");

  // Initialize WISMO228
  wismo.init();
  
  Serial.println("Powering up, please wait...");
  
  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    Serial.println("TraLog is awake!");
    Serial.println("Let's ping www.google.com!");

    // Connect to GPRS network
    if (wismo.openGPRS(apn, username, password))
    {
      Serial.println("Connected to GPRS.");

      // Ping server and print response time in ms
      // 0 ms will be returned if ping failed
      Serial.print("Ping result: ");
      Serial.print(wismo.ping("www.google.com"));
      Serial.println(" ms");

      // Close the GPRS connection
      if (wismo.closeGPRS())
      {
        Serial.println("GPRS connection closed.");
      }
    }
    else
    {
      Serial.println("GPRS connection failed.");
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