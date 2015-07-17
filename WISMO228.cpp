/*******************************************************************************
* WISMO228 Library
* Version: 1.20
* Date: 12-05-2013
* Company: Rocket Scream Electronics
* Author: Lim Phang Moh
* Website: www.rocketscream.com
*
* This is an Arduino compatible library for WISMO228 GSM-GPRS module. 
* It is to be used together with our TraLog shield. Please check our wiki 
* (www.rocketscream.com/wiki) for more information on using this piece of 
* library together with the TraLog shield.
*
* This firmware owed very much on the works of other talented individual as
* follows:
* ==========================================
* Mikal Hart (www.arduiniana.org)
* ==========================================
* Author of Arduino NewSoftSerial (incorporated into Arduino IDE 1.0 as Software 
* Serial) library. We won't be having enough serial ports on the Arduino without
* the work of Mikal!
*
* This library is licensed under Creative Commons Attribution-ShareAlike 3.0 
* Unported License. 
*
* Revision  Description
* ========  ===========
* 1.20      Added support for hardware serial (Serial, Serial1, Serial2, &
*           Serial3) usage.
*           SoftwareSerial object needs to be declared outside of library.
*
* 1.10      Reduce the amount of RAM usage by moving all AT command responses
*           from WISMO228 into flash which allows the complete use of the 
*						"stream find" function.
*           Added HTTP PUT method which can be used to upload data to Cosm or
*           Pachube. Example are also added.
*           Added retry mechanism to connect to GPRS.
*           Added retry mechanism to open a port with remote server.
*           Removed power up delay and replaced with module ready query.
*
* 1.00      Initial public release. Tested with L22 & L23 of WISMO228 firmware.
*						Only works with Arduino IDE 1.0 & 1.0.1.
*******************************************************************************/
// ***** INCLUDES *****

#include "WISMO228.h"

