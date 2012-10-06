/*******************************************************************************
* WISMO228 Library - Send Email Example
* Version: 1.00
* Date: 28-03-2012
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an example on sending an email using the WISMO228 library on the 
* TraLog Shield.
*
* ============
* Requirements
* ============
* 1. UART selection switch to SW position (uses pin D5 (RX) & D6 (TX)).
* 2. Jumper J14 is closed to allow usage of pin A2 to control on-off state of 
*    WISMO228 module.
* 3. You need to know your service provider APN name, username, and password. If
*    they don't specify the username and password, you can use " ". Notice the
*    space in between the quote mark.
* 4. You also need to know your SMTP server name and port number.
* 5. The email password is in plain text and will be converted into Base-64 
*    format by the library internally.
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
const  char  apn[] = "apn";
const  char  username[] = "username";
const  char  password[] = "password";

// ***** VARIABLES *****
bool	status;

// WISMO228 class
WISMO228  wismo(rxPin, txPin, onOffPin);

void setup()  
{
  // Initialize WISMO228 module start up status
  status = false;

  Serial.begin(9600);
  Serial.println("Sending Email Example");

  // Initialize WISMO228
  wismo.init();
  
  Serial.println("Powering up, please wait...");
  
  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    Serial.println("TraLog is awake!");
    Serial.println("Let's send some email!");

    // Connect to GPRS network
    if (wismo.openGPRS(apn, username, password))
    {
      Serial.println("Connected to GPRS.");
      Serial.println("Sending email, please wait...");
      
      // Send email
      if (wismo.sendEmail("mail.domain.com", 
      "25", "sender@domain.com", "password", 
      "recipient@somedomain.com", "Hello World", "TraLog"))
      {
        Serial.println("Email sent!");
      }			
      else
      {
        Serial.println("Email sending failed!");
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