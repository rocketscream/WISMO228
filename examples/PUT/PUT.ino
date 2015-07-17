/*******************************************************************************
* WISMO228 Library - HTTP GET Example
* Version: 1.10
* Date: 09-07-2013
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an example on how to update a Cosm feed using HTTP PUT request over 
* GPRS.
*
* ============
* Requirements
* ============
* 1. UART selection switch to SW position (uses pin D5 (RX) & D6 (TX)).
* 2. On v1 of the shield, jumper J14 is closed to allow usage of pin A2 to 
*    control on-off state of WISMO228 module. On v2 of the shield, short the 
*    jumper labelled A2 & GSM-ON. This is the default factory setting.
* 3. You need to know your service provider APN name, username, and password. 
*    If they don't specify the username and password, you can use " ". Notice 
*    the space in between the quote mark. 
* 4. The Cosm server name & port.
* 5. Your Cosm api key and data stream name. 
* 6. An analog sensor connected to pin A5. You can use an LDR for this example.
*
* ============
* Instructions
* ============
* 1. Please create your own Cosm/Pachube datastream and replace necessary
*    parameter(s) to suit your setup. It's very easy to add more data stream.
*
* This example is licensed under Creative Commons Attribution-ShareAlike 3.0 
* Unported License. 
*
* Revision  Description
* ========  ===========
* 1.10      Updated to support WISMO228 Library version 1.20.
*           Tested up to Arduino IDE 1.0.4.
* 1.00      Initial public release. Tested with L22 & L23 of WISMO228 firmware.
*           Only works with Arduino IDE 1.0 & 1.0.1. Uses v1.10 of the WISMO228 
*           library.
******************************************************************************/
// ***** COMPILE OPTIONS *****
#define DEBUG 

// ***** INCLUDES *****
//#include "SoftwareSerial.h"
#include <WISMO228.h>

// ***** PIN ASSIGNMENT *****
const  uint8_t  gsmRxPin = 5;
const  uint8_t  gsmTxPin = 6;
const  uint8_t  gsmOnOffPin = A2;
const  uint8_t  sensorPin = A5;

// ***** CONSTANTS *****
// ***** GPRS PARAMATERS *****
const  char  apn[] = "InsertYourApn";
const  char  username[] = "InserYourUsername";
const  char  password[] = "InsertYourPassword";
// ***** COSM PARAMATERS *****
const  char server[] = "api.cosm.com";
const  char path[] = "/v2/feeds/InsertYourCosmFeedNumberHere.csv";	
const  char port[] = "80";
const  char host[] = "api.cosm.com";
const  char controlKey[] = "X-PachubeApiKey: InsertYourCosmApiKeyNumberHere";
const  char contentType[] = "text/csv";
const  char stream[] = "InsertYourDataStreamNameHere";
#define UPDATE_INTERVAL 60000

// ***** CLASSES *****
// Software serial class
//SoftwareSerial gsm(gsmRxPin, gsmTxPin); 
// WISMO228 class
//WISMO228  wismo(&gsm, gsmOnOffPin);
WISMO_CREATE_DEFAULT_INSTANCE()

// ***** VARIABLES *****
char  data[32];
unsigned long scheduler;

void setup()  
{
  // Use hardware serial to track the progress of task execution
  #ifdef DEBUG
	Serial.begin(115200);
	while (!Serial) { ; } // wait for serial port to connect. Needed for Leonardo only
	Serial.println(F("HTTP PUT Example"));
		Serial.println(F("Powering up GSM, please wait..."));
  #endif

  // Initialize WISMO228
  wismo.init();

  // Perform WISMO228 power up sequence
  if (wismo.powerUp())
  {
    #ifdef DEBUG
      Serial.println(F("GSM is awake."));
			Serial.print(F("Update interval: "));
			Serial.print(UPDATE_INTERVAL);
			Serial.println(F(" ms"));
    #endif
  }
  
  // Initialize sensor reading & feed interval
  scheduler = millis();
}

void loop() 
{ 
  if (millis() > scheduler)
  {
    scheduler = millis() + UPDATE_INTERVAL;
    
    // Feed current sensor reading to Cosm datastream
    processGsm();
  }
}

void processGsm(void)
{
  char  buffer[4]; 

  #ifdef DEBUG
    Serial.println();
    Serial.println(F("Let's send some sensor data."));
  #endif
		
  // Read sensor & convert from int to string
  itoa(analogRead(sensorPin), buffer, 10);
  // Prepare data for Cosm feed
  strcpy(data, stream);
  strcat(data, ",");
  strcat(data, buffer);
  strcat(data, "\r\n");
  // Terminate the string
  strcat(data, "\0");
    
  #ifdef DEBUG
    Serial.print(data);
  #endif
		
  // Connect to GPRS network
  if (wismo.openGPRS(apn, username, password))
  {
    #ifdef DEBUG
      Serial.println(F("GPRS OK."));
      Serial.println(F("Sending feed to Cosm, please wait..."));      
    #endif
      
    // Send current sensor reading as Cosm feed through HTTP PUT request
    if (wismo.putHttp(server, path, port, host, data, controlKey, 
		                  contentType))			
    {
      #ifdef DEBUG
        Serial.println(F("Feed sent!"));
      #endif
    }
    else
    {
      #ifdef DEBUG
        Serial.println(F("Unable to send feed."));
      #endif
    }

    // Close the GPRS connection
    if (wismo.closeGPRS())
    {
      #ifdef DEBUG
        Serial.println(F("GPRS closed."));
      #endif
    }
  }
  else
  {
    #ifdef DEBUG
      Serial.println(F("GPRS failed."));
    #endif
  }
}

