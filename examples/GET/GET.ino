/*******************************************************************************
* WISMO228 Library - HTTP GET Example
* Version: 1.00
* Date: 28-03-2012
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an example on HTTP GET method using the WISMO228 library on the 
* TraLog Shield. The goal of this example is to send data to a server using the
* HTTP GET method of 2 data field (name & country code). The country code must
* a valid 2 letters ISO3166 country code and any name submitted with links will
* be rejected. Despite it's name "GET", we are not retrieving a website but it 
* is possible although this is rather limited by the amount of RAM on the 
* Arduino.
*
* ============
* Requirements
* ============
* 1. UART selection switch to SW position (uses pin D5 (RX) & D6 (TX)).
* 2. Jumper J14 is closed to allow usage of pin A2 to control on-off state of 
*    WISMO228 module. This is the default factory setting.
* 3. You need to know your service provider APN name, username, and password. If
*    they don't specify the username and password, you can use " ". Notice the
*    space in between the quote mark. 
* 4. The server hosting the PHP script to service the HTTP GET request:
*    www.xetroc.com
* 5. More details on the server side implementation is documentated on that 
*    server.
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
const  char  apn[] = "apn";
const  char  username[] = "username";
const  char  password[] = "password";

// WISMO228 class
WISMO228  wismo(rxPin, txPin, onOffPin);

void setup()  
{
  // Webpage content buffer
  char  message[50];
  
  Serial.begin(9600);
  Serial.println("HTTP Get Example");

  // Initialize WISMO228
  wismo.init();
  
  Serial.println("Powering up, please wait...");
  
  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    Serial.println("TraLog is awake!");
    Serial.println("Let's send some data!");

    // Connect to GPRS network
    if (wismo.openGPRS(apn, username, password))
    {
      Serial.println("Connected to GPRS.");
      
      if (wismo.getHttp("www.xetroc.com", 
			                  "/getData.php?name=Dean&countryCode=GB",
												"80", message, 50))
      {
        Serial.println("Webpage retrieved:");
        Serial.println(message);
        Serial.println("Check www.xetroc.com/get.php to see the result!");
      }
      else
      {
        Serial.println("Hard luck, server might be busy.");
      }

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