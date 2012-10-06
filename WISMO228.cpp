/*******************************************************************************
* WISMO228 Library
* Version: 1.10
* Date: 06-10-2012
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
#include <Arduino.h>

// ***** CONSTANTS *****
// ***** EXPECTED WISMO228 RESPONSE *****
prog_char ok[] PROGMEM = "OK"; 
prog_char simOk[] PROGMEM = "\r\n+CPIN: READY\r\n\r\nOK\r\n";
prog_char networkOk[] PROGMEM = "\r\n+CREG: 0,1\r\n\r\nOK\r\n";
prog_char smsCursor[] PROGMEM = "\r\n> ";
prog_char smsSendOk[] PROGMEM = "\r\n+CMGS: ";
prog_char smsList[] PROGMEM = "\r\n+CMGL: ";
prog_char smsUnread[] PROGMEM = "\"REC UNREAD\",\"+";
prog_char commaQuoteMark[] PROGMEM = ",\"";
prog_char quoteMark[] PROGMEM = "\"";
prog_char newLine[] PROGMEM = "\r\n";
prog_char carriegeReturn[] PROGMEM = "\r";
prog_char lineFeed[] PROGMEM = "\n";
prog_char pingOk[] PROGMEM = "\r\nOK\r\n\r\n+WIPPING: 0,0,";
prog_char clockOk[] PROGMEM = "\r\n+CCLK: \"";
prog_char rssiCheck[] PROGMEM = "\r\n+CSQ: ";
prog_char portOk[] PROGMEM = "\r\nOK\r\n\r\n+WIPREADY: 2,1\r\n";
prog_char connectOk[] PROGMEM = "\r\nCONNECT\r\n";
prog_char dataOk[] PROGMEM = "\r\n+WIPDATA: 2,1,";
prog_char usernamePrompt[] PROGMEM = "334 VXNlcm5hbWU6\r\n";
prog_char passwordPrompt[] PROGMEM = "334 UGFzc3dvcmQ6\r\n";
prog_char authenticationOk[] PROGMEM = "235 Authentication succeeded\r\n";
prog_char senderOk[] PROGMEM = "250 OK\r\n";
prog_char recipientOk[] PROGMEM = "250 Accepted\r\n";
prog_char emailInputPrompt[] PROGMEM = "354 ";
prog_char emailSent[] PROGMEM = "250 OK ";
prog_char shutdownLink[] PROGMEM = "SHUTDOWN";

// ***** BASE64 ENCODING TABLE *****
prog_uchar	base64Table[] PROGMEM =	{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
																			"abcdefghijklmnopqrstuvwxyz"
																			"0123456789+/"};

// ***** VARIABLES *****
char responseBuffer[RESPONSE_LENGTH_MAX];

WISMO228::WISMO228(unsigned char rxPin, unsigned char txPin, 
									 unsigned char onOffPin): uart(rxPin, txPin)
{
	_onOffPin = onOffPin;
	_ringPin = NC;
}

WISMO228::WISMO228(unsigned char rxPin, unsigned char txPin, 
									 unsigned char onOffPin, 
									 unsigned char ringPin,
									 void (*newSmsFunction)(void)): uart(rxPin, txPin)
{
	_onOffPin = onOffPin;
	
	// If either digital pin 2 or 3 is used as RING pin
	if ((ringPin == 2) || (ringPin == 3))
	{
		_ringPin = ringPin;
		functionPtr = newSmsFunction;
	}
	else
	{
		// Invalid external interrupt pin
		_ringPin = NC;
	}
}

/*******************************************************************************
* Name: getStatus
* Description: WISMO228 operation status.
*
* Argument  			Description
* =========  			===========
* 1. NIL				
*
* Return					Description
* =========				===========
* 1. status				Status of the WISMO228 operation.
*
*******************************************************************************/
status_t	WISMO228::getStatus()
{
	return (status);
}

/*******************************************************************************
* Name: init
* Description: Module parameters and pins initialization.
*
* Argument  			Description
* =========  			===========
* 1. NIL				
*
* Return					Description
* =========				===========
* 1. NIL
*
*******************************************************************************/
void	WISMO228::init()
{
	pinMode(_onOffPin, OUTPUT);
	digitalWrite(_onOffPin, LOW);
	
	if (_ringPin != NC)
	{
		pinMode(_ringPin, INPUT);
	}
	
	// Initial WISMO228 state
	status = OFF;
}

/*******************************************************************************
* Name: init
* Description: Perform WISMO228 power up sequence and configurations.
*
* Argument  			Description
* =========  			===========
* 1. NIL				
*
* Return					Description
* =========				===========
* 1. success			Returns true if WISMO228 successfully powers up or false 
*									if otherwise.
*
*******************************************************************************/
bool	WISMO228::powerUp()
{
	bool	success = false;
	
	if (status == OFF)
	{
		// Start software UART
		uart.begin(BAUD_RATE);
		
		if (_onOffPin != NC)
		{
			digitalWrite(_onOffPin, HIGH);
			// 685 ms period is required
			delay(685);
			digitalWrite(_onOffPin, LOW);
			// WISMO228 is powered up
			status = ON;
		}
				
		// Remove power up invalid characters
		uart.flush();
		// Configure reply timeout period
		uart.setTimeout(MIN_TIMEOUT);
		
		// Turn echo off to ease serial congestion
		if (offEcho())
		{
			// Check SIM card initialization process
			if (simReady())	
			{
				// Check network registeration status
				if (registerNetwork())
				{
					// Enable text mode SMS
					if (textModeSms())
					{
						// If ring pin used as new SMS indicator
						if (_ringPin != NC)
						{
							// Configure ring pin as new SMS indicator
							if (newSmsSetup())
							{
								success = true;
							}
						}
						else	success = true;
					}
				}
			}
		}
	}
	
	return (success);
}

/*******************************************************************************
* Name: shutdown
* Description: Shut down the WISMO228 module and related interrupt if used.
*
* Argument  			Description
* =========  			===========
* 1. NIL				
*
* Return					Description
* =========				===========
* 1. NIL					
*
*******************************************************************************/
void	WISMO228::shutdown()
{
	if (status == ON)
	{
		// If ring pin is used as new SMS indicator
		if (_ringPin != NC)
		{
			// Disable interrupt on DTR pin
			detachInterrupt(_ringPin - 2);
		}
		
		digitalWrite(_onOffPin, LOW);
		delay(100);
		digitalWrite(_onOffPin, HIGH);
		delay(3000);										// Stated as 5500 ms in datasheet, 
		digitalWrite(_onOffPin, LOW);		// but 3000 ms works fine

		uart.flush();
		uart.end();
		// WISMO228 is in shutdown mode
		status = OFF;
	}
}

/*******************************************************************************
* Name: simReady
* Description: Checks whether SIM card is ready.
*
* Argument  			Description
* =========  			===========
* 1. NIL				
*
* Return					Description
* =========				===========
* 1. success 			Returns true if SIM card is successfully configured or false 
*									if otherwise.
*
*******************************************************************************/
bool	WISMO228::simReady()
{
	bool	success = false;
	unsigned long	timeout;
	
	readFlash(simOk, responseBuffer);
	timeout = millis() + MAX_TIMEOUT;
		
	// Wait for SIM card initialization
	while (timeout > millis())
	{
		uart.println(F("AT+CPIN?"));

		if (uart.find(responseBuffer))
		{
			success = true;
			break;
		}	
	}

	return (success);
}

/*******************************************************************************
* Name: offEcho
* Description: Configure AT command to operate without echo.
*
* Argument  			Description
* =========  			===========
* 1. NIL
*
* Return					Description
* =========				===========
* 1. success 			Returns true if AT command without echo is successfully 
*									configured or false if otherwise.
*
*******************************************************************************/
bool	WISMO228::offEcho()
{
	bool	success = false;
	unsigned long	timeout;
	
	readFlash(ok, responseBuffer);

	timeout = millis() + MAX_TIMEOUT;
		
	// Try to turn off echo upon power up (some unknown carrier setup message 
	// might be available)
	while (timeout > millis())
	{
		uart.println(F("ATE0"));
			
		if (uart.find(responseBuffer))
		{	
			success = true;
			break;
		}
	}
	return (success);
}

/*******************************************************************************
* Name: registerNetwork
* Description: Checks whether module is successfully registered to the network.
*
* Argument  			Description
* =========  			===========
* 1. NIL
*
* Return					Description
* =========				===========
* 1. success 			Returns true if module is successfully registered to the
*									network or false if otherwise.
*
*******************************************************************************/
bool	WISMO228::registerNetwork()
{
	bool	success = false;
	unsigned long	timeout;
	
	timeout = millis() + MAX_TIMEOUT;
		
	// Wait for network registeration
	while (timeout > millis())
	{
		uart.println(F("AT+CREG?"));
		
		readFlash(networkOk, responseBuffer);

		if (uart.find(responseBuffer))
		{	
			success = true;
			break;
		}
	}
	
	return (success);
}

/*******************************************************************************
* Name: textModeSms
* Description: Configure SMS module in text mode.
*
* Argument  			Description
* =========  			===========
* 1. NIL
*
* Return					Description
* =========				===========
* 1. success 			Returns true if SMS text mode is successfully configured or 
*									false if otherwise.
*
*******************************************************************************/
bool WISMO228::textModeSms()
{
	bool	success = false;
		
	// Text mode SMS
	uart.println(F("AT+CMGF=1"));
	
	readFlash(ok, responseBuffer);
	
	if (uart.find(responseBuffer))
	{
		success = true;
	}
	
	return (success);
}

/*******************************************************************************
* Name: newSmsSetup
* Description: Configure new SMS notification setup.
*
* Argument  			Description
* =========  			===========
* 1. NIL
*
* Return					Description
* =========				===========
* 1. success 			Returns true if DTR pin is successfully configured as new
*									SMS indication or false if otherwise.
*
*******************************************************************************/
bool WISMO228::newSmsSetup()
{
	bool	success = false;
	
	// User RING pin (falling edge) as new message indication
	uart.println(F("AT+PSRIC=2,0"));
	
	readFlash(ok, responseBuffer);
	
	if (uart.find(responseBuffer))
	{
		// Attach interrupt for RING pin as new SMS indicator
		attachInterrupt((_ringPin - 2), functionPtr, FALLING); 

		success = true;
	}
	
	return (success);
}

/*******************************************************************************
* Name: sendSms
* Description: Send SMS.
*
* Argument  			Description
* =========  			===========
* 1. recipient    Phone number of the recipient.
*
* 2. message			Message of length not more than 160 characters.		
*
* Return					Description
* =========				===========
* 1. success 			Returns true if SMS is successfully sent and false if
*									otherwise.
*
*******************************************************************************/		
bool	WISMO228::sendSms(const char *recipient, const char *message)
{
	bool success = false;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	// If WISMO228 is powered on
	if (status == ON)
	{
		// If SMS length is less than 160 characters
		if (strlen(message) <= SMS_LENGTH_MAX)
		{
			// Clear any unwanted data in UART
			uart.flush();
			uart.print(F("AT+CMGS=\""));
			uart.print(recipient);
			uart.println(F("\""));
			
			// Writing SMS takes more time compared to other task
			uart.setTimeout(MED_TIMEOUT);
			
			readFlash(smsCursor, responseBuffer);
			
			// Wait for SMS writing prompt (cursor)
			if (uart.find(responseBuffer))
			{
				// Write the message
				uart.print(message);
				// End the message
				uart.write(26);
				
				readFlash(smsSendOk, responseBuffer);
				
				if (uart.find(responseBuffer))
				{
					// Revert back to normal reply timeout
					uart.setTimeout(MIN_TIMEOUT);
					
					if (waitForReply(3, MIN_TIMEOUT))
					{
						// Read out all SMS ID
						while ((char)uart.read() != '\r');
						
						readFlash(ok, responseBuffer);
						
						if (uart.find(responseBuffer))
						{
							success = true;
						}
					}
				}
			}
		}
	}
	return (success);	
}

/*******************************************************************************
* Name: readSms
* Description: Read 1 new SMS.
*
* Argument  			Description
* =========  			===========
* 1. sender				Sender of the received SMS.
*
*	2. message			Content of the SMS.
*
* Return					Description
* =========				===========
* 1. success 			Returns true if new SMS retrieval is successfully or false if 
*									otherwise.
*
*******************************************************************************/
bool WISMO228::readSms(char *sender, char *message)
{
	bool success = false;
	unsigned	char	rxCount;
	char	index[4];
	char	*indexPtr;
	char	rxByte;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Retrieve unread SMS
		uart.println(F("AT+CMGL=\"REC UNREAD\""));
		
		readFlash(smsList, responseBuffer);

		if (uart.find(responseBuffer))
		{
			// Maximum index is 255 (3 characters)
			// 4th character is ","
			if (waitForReply(4, MIN_TIMEOUT))
			{
				// Initialize the SMS index string
				indexPtr = index;
				// Retrieve the 1st digit
				rxByte = uart.read();
				
				// Save the SMS index number for deleting purposes 
				while (rxByte != ',')
				{
					*indexPtr++ = rxByte;
					rxByte = uart.read();
				}
				// Terminate the index string
				*indexPtr = '\0';
				
				readFlash(smsUnread, responseBuffer);
				
				if (uart.find(responseBuffer))
				{			
					do
					{
						// We are running at lightning speed, wait for a while
						while (!uart.available());
						rxByte = uart.read();
						if (rxByte == '"')	break;
						*sender++ = rxByte;
					} while (rxByte != '"');
					
					// Terminate the sender string
					*sender = '\0';
					
					readFlash(commaQuoteMark, responseBuffer);

					if (uart.find(responseBuffer))
					{
						do
						{
							// We are running at lightning speed, wait for a while
							while (!uart.available());
							rxByte = uart.read();
						}
						while (rxByte != '"');
						
						if (uart.find(responseBuffer))
						{
							// We are running at ligthning speed, have to wait for data
							if (waitForReply(CLOCK_COUNT_MAX, MIN_TIMEOUT))
							{
								// Retrieve time stamping 
								for (rxCount = CLOCK_COUNT_MAX; rxCount > 0; rxCount--)
								{
									// Can be used for time reference
									rxByte = uart.read();
								}
								
								readFlash(quoteMark, responseBuffer);
								
								if (uart.find(responseBuffer))
								{
									readFlash(newLine, responseBuffer);

									if (uart.find(responseBuffer))
									{
										if (waitForReply(1, MIN_TIMEOUT))
										{
											rxByte = uart.read();
							
											// Message can be empty
											while (rxByte != '\r')
											{
												// Save SMS message
												*message++ = rxByte;
												// We are running faster than incoming data
												while (!uart.available());
												rxByte = uart.read();
											}
							
											// Terminate the message string
											*message = '\0';
											readFlash(ok, responseBuffer);

											if (uart.find(responseBuffer))
											{
												// Delete the SMS to avoid memory overflow
												uart.print(F("AT+CMGD="));
												uart.println(index);
								
												if (uart.find(responseBuffer))
												{
													success = true;
												}
											}
										}
									}	
								}
							}
						}
					}
				}
			}
		}
	}
	
	return (success);
}

/*******************************************************************************
* Name: openGPRS
* Description: Get the WISMO228 to connect to the GPRS network.
*
* Argument  			Description
* =========  			===========
* 1. apn     			Access Point Name (APN) of the IP Packet Data Network (PDN) 
*									that you want to connect to.
*
* 2. username			Username of the APN you are connecting to. If no username is
*									is required, use the " " to indicate this.
*
* 3. password			Password of corresponding username. If no password is required,
*									use the " " string to indicate this.		
*
*******************************************************************************/
bool WISMO228::openGPRS(const char *apn, const char *username, 
												const char *password)
{
	bool success = false;
	unsigned char attempt;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Start TCP/IP stack
		uart.println(F("AT+WIPCFG=1"));
		
		readFlash(ok, responseBuffer);

		if (uart.find(responseBuffer))
		{
			// Open GPRS bearer
			uart.println(F("AT+WIPBR=1,6"));
			
			if (uart.find(responseBuffer))
			{
				// Set the APN
				uart.print(F("AT+WIPBR=2,6,11,\""));
				uart.print(apn);
				uart.println(F("\""));
				
				if (uart.find(responseBuffer))
				{
					// Set the username
					uart.print(F("AT+WIPBR=2,6,0,\""));
					uart.print(username);
					uart.println(F("\""));
					
					if (uart.find(responseBuffer))
					{
						// Set the password
						uart.print(F("AT+WIPBR=2,6,1,\""));
						uart.print(password);
						uart.println(F("\""));
						
						if (uart.find(responseBuffer))
						{
							// It takes slightly longer to start GPRS bearer 	
							uart.setTimeout(MED_TIMEOUT);						
							
							// Maximum 3 attempt to connect to GPRS as base station might not
              // have enough time slots for GPRS as voice call is given priority
							for (attempt = 3; attempt > 0; attempt--)
							{
								// Start GPRS bearer
								uart.println(F("AT+WIPBR=4,6,0"));
							
								if (uart.find(responseBuffer))
								{
									// GPRS connection is up
									status = GPRS_ON;
									success = true;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	
	return (success);
}

/*******************************************************************************
* Name: closeGPRS
* Description: Get the WISMO228 to disconnect from the GPRS network.
*
* Argument  			Description
* =========  			===========
* 1. NIL	
*
* Return					Description
* =========				===========
* 1. success 			Returns true if successfully disconnected from GPRS network or
*								  false if otherwise.
*
*******************************************************************************/
bool WISMO228::closeGPRS()
{
	bool success = false;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	// Only close GPRS connection if currently on
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Stop TCP/IP stack (automatically detach from GPRS)
		uart.println(F("AT+WIPCFG=0"));
		
		readFlash(ok, responseBuffer);
		
		if (uart.find(responseBuffer))
		{
			// Revert to on mode
			status = ON;
			success = true;
		}	
	}
	
	return (success);
}

/*******************************************************************************
* Name: ping
* Description: Ping a server.
*
* Argument  			Description
* =========  			===========
* 1. url     			URL of the server to ping.
*									Example: www.google.com, 200.200.200.200
*	
* Return					Description
* =========				===========
* 1. responseTime The response time of ping in ms. If no response from the 
*									server or timeout occurs, a value of 0 is return.
*
*******************************************************************************/
unsigned int	WISMO228::ping(const char	*url)
{	
	// Ping response time
	unsigned int	responseTime = 0;
	char	responseTimeStr[RESPONSE_TIME_MAX];
	char	*responseTimeStrPtr;
	char	rxByte;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	// Needs GPRS connection to execute ping
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Execute the PING service
		uart.print(F("AT+WIPPING=\""));
		uart.print(url);
		uart.println(F("\""));

		readFlash(pingOk, responseBuffer);

		// If ping is successful
		if (uart.find(responseBuffer))
		{
			// Initialize string of response time array
			responseTimeStrPtr = responseTimeStr;
				
			do
			{
				while (!uart.available());
				rxByte = uart.read();
				*responseTimeStrPtr++ = rxByte;
			}	while (rxByte != '\r');
					
			// Terminate the string
			*responseTimeStrPtr = '\0';
				
			readFlash(lineFeed, responseBuffer);	
	
			if (uart.find(responseBuffer))
			{
				// Convert port number into string
				sscanf(responseTimeStr, "%u", &responseTime);
			}
		}
	}
	
	return(responseTime);
}

/*******************************************************************************
* Name: getHttp
* Description: Perform HTTP method GET to retrieve or send data to a server.
*
* Argument  			Description
* =========  			===========
* 1. url     			URL of the server.
*									Example: www.google.com, 200.200.200.200
*
* 2. path					The path or directory to access in the server. 
*									Examples: 
*									"/" - No directory is required.
*									"/directory/subdirectory" - Directory is required.
*									"/script.php?data=value" - For sending data to server.
*
*	3. port					Server TCP port number from 0-65535.
*
*	4. *message			Location to store the data retrieved from the server.
*
*	5. limit				The maximum length of data to received from the server. 
*	
* Return					Description
* =========				===========
* 1. success 			Returns true if the procedure is successful and false if 
*									otherwise.
*
*******************************************************************************/
bool	WISMO228::getHttp(const char *server, const char *path, const char	*port, 
												char *message, unsigned int limit)
{
	unsigned long timeout;
	char	rxByte;
	bool	success = false;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	// If currently attach to GPRS
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		
		if (openPort(server, port))
		{			
			if (exchangeData())
			{
				uart.print(F("GET "));
				uart.print(path);
				uart.print(F(" HTTP/1.1\r\nHost: "));
				uart.print(server);
				uart.println(F("\r\n"));
				
				timeout = millis() + 3000;
				
				while ((limit > 0) && (millis() < timeout))
				{
					if (uart.available())
					{
						*message++ = uart.read();
						limit--;
					}
				}

				delay(3000);
				uart.flush();
				
				// Revert back to AT command mode
				uart.print(F("+++"));
				
				// Expecting an "OK" response
				readFlash(ok, responseBuffer);
				
				if (uart.find(responseBuffer))
				{								
					// Close the TCP socket with server
					uart.println(F("AT+WIPCLOSE=2,1"));
					
					// Expecting an "OK" response
					if (uart.find(responseBuffer))
					{
						success = true;
					}
				}
			}
		}
	}
	
	// Flush remaining unrecognized characters
	uart.flush();
	
	return (success);
}

/*******************************************************************************
* Name: putHttp
* Description: Perform HTTP method PUT to send data to a server.
*
* Argument  			Description
* =========  			===========
* 1. server     	URL of the server.
*									Example: www.google.com, 200.200.200.200
*
* 2. path					The path or directory to access in the server. 
*									Examples: 
*									"/" - No directory is required.
*									"/directory/subdirectory" - Directory is required.
*									"/script.php?data=value" - For sending data to server.
*
*	3. port					Server TCP port number from 0-65535.
*
*	4. host					Host of the residing end application. Typically same as the 
*									server.
*
*	5. data				  Data in the following format:
*									dataName1,value\r\ndataName2,value\r\n
*									Each line consist of a data name separated by a comma and it's
*									corresponding value. The new line feed is required for each
*									data entry.
*	6. controlKey		Control key name and it's value which is specified by the 
*									server hosting the application in the following format:
*									ControlKeyName: Value					 
*
* 7. contentType	Type of content which is being sent over.
*	
* Return					Description
* =========				===========
* 1. success 			Returns true if the procedure is successful and false if 
*									otherwise.	
*
*******************************************************************************/
bool	WISMO228::putHttp(const char *server, const char *path, const char *port, 
              const char *host, const char *data, const char *controlKey, 
							const char *contentType)
{
	bool	success = false;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	// If currently attach to GPRS
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();

		// Open a port with remote server	
		if (openPort(server, port))
		{		
			// Enter transparent data mode
			if (exchangeData())
			{
				uart.print(F("PUT "));
				uart.print(path);
				uart.println(F(" HTTP/1.1"));
				uart.print(F("Host: "));
				uart.println(host);
				uart.println(controlKey);
				uart.print(F("Content-Length: "));
				uart.println(strlen(data));
				uart.print(F("Content-Type: "));
				uart.println(contentType);
				uart.println(F("Connection: close"));
				uart.println();
				uart.print(data);
				uart.print("\n");
				
				// If you need to retrieve the whole response from the remote server,
				// retrieve the response starting from here
				
				// It takes more time for server to response to a PUT request	
				uart.setTimeout(MED_TIMEOUT);
				// Expecting an OK from remote server
				readFlash(ok, responseBuffer);
				
				// If HTTP PUT request successful
				if (uart.find(responseBuffer))
				{
					// Expecting a SHUTDOWN signal from remote server
					readFlash(shutdownLink, responseBuffer);
					
					if (uart.find(responseBuffer))
					{
						// PUT request completed
						success = true;
					}
				}
				// Revert back to normal reply timeout
				uart.setTimeout(MIN_TIMEOUT);
						
				// Revert back to AT command mode
				uart.print(F("+++"));
				
				// Expecting an OK from remote server
				readFlash(ok, responseBuffer);
						
				if (uart.find(responseBuffer))
				{
					// Close the TCP socket with server
					uart.println(F("AT+WIPCLOSE=2,1"));
					
					// Expecting an "OK" response
					if (uart.find(responseBuffer))
					{
						// Port properly closed
					}
				}
			}
			else
			{
				uart.println(F("AT+WIPCLOSE=2,1"));
				
				// Expecting an "OK" response
				if (uart.find(responseBuffer))
				{
					// Port properly closed
				}
			}	
		}
	}
	
	return (success);
}

/*******************************************************************************
* Name: sendEmail
* Description: Send an email through SMTP server.
*
* Argument  			Description
* =========  			===========
* 1. smtpServer   SMTP server name.
*									Example: mail.yourdomain.com
*
* 2. port					SMTP port number. Usually is 25.
*
*	3. username			Complete username (email address).
*									Example: user@yourdomain.com
*
*	4. password			Corresponding password for the username.
*
*	5. recipient		Recipient email address.
*									Example: recipient@somedomain.com
*
*	6. title				Title or subject of the email.
*									Example: Hello World!
*
*	7. message			Content of the email.
*	
* Return					Description
* =========				===========
* 1. success 			Returns true if the procedure is successful and false if 
*									otherwise.
*
*******************************************************************************/	
bool WISMO228::sendEmail(const char *smtpServer, const char *port, 
												 const char *username, const char *password, 
												 const char *recipient, const char *title, 
												 const char *content)
{
	bool	success = false;
	char	rxByte;
	char	dataCountStr[5];
	char	*dataCountStrPtr;
	char	base64[50];
	unsigned	int	dataCount;
	//unsigned long	timeout;

	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	// If currently attach to GPRS
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		
		if (openPort(smtpServer, port))
		{
			readFlash(dataOk, responseBuffer);
			
			// Retrieving data from web takes longer time
			uart.setTimeout(MED_TIMEOUT);
			
			if (uart.find(responseBuffer))
			{
				dataCountStrPtr = dataCountStr;
				
				do
				{
					while (!uart.available());
					rxByte = uart.read();
					*dataCountStrPtr++ = rxByte;
				} while (rxByte != '\r');
				
				// Terminate the string
				*dataCountStrPtr = '\0';
				
				// Convert data count into unsigned integer
				sscanf(dataCountStr, "%u", &dataCount);
				
				uart.setTimeout(MIN_TIMEOUT);
				
				readFlash(lineFeed, responseBuffer);
				
				if (uart.find(responseBuffer))
				{			
					if (exchangeData())
					{
						for (dataCount; dataCount > 0; dataCount--)
						{
							// We are running at the speed of light!
							while (!uart.available());
							// Pull out the server messages
							rxByte = uart.read();
						}
						// Start communicating with server using extended SMTP protocol
						uart.println(F("EHLO"));
						
						delay (5000);

						uart.flush();
				
						// Initiate account login
						uart.println(F("AUTH LOGIN"));
						
						// Retrieve string of username prompt in base 64 format from flash
						readFlash(usernamePrompt, responseBuffer);
							
						// If receive the username prompt in base 64 format
						if (uart.find(responseBuffer))
						{
							// Encode username into base 64 format
							encodeBase64(username, base64);
							// Send username in base 64 format
							uart.println(base64);
							
							// Retrieve string of password prompt in base 64 format from flash
							readFlash(passwordPrompt, responseBuffer);
							
							// If receive the password prompt in base 64 format
							if (uart.find(responseBuffer))
							{
								// Encode username into base 64 format
								encodeBase64(password, base64);		
								// Send password in base 64 format
								uart.println(base64);
								
								// Retrieve string of authentication success in base 64 format 
								// from flash
								readFlash(authenticationOk, responseBuffer);
								
								// If receive authentication success
								if (uart.find(responseBuffer))
								{
									// Email sender
									uart.print(F("MAIL FROM: "));
									uart.println(username);
									
									readFlash(senderOk, responseBuffer);
									
									// If sender ok
									if (uart.find(responseBuffer))
									{
										// Email recipient
										uart.print(F("RCPT TO: "));
										uart.println(recipient);
										
										readFlash(recipientOk, responseBuffer);
										
										// If recipient ok
										if (uart.find(responseBuffer))
										{
											// Start of email body
											uart.println(F("DATA"));
											
											readFlash(emailInputPrompt, responseBuffer);
											
											// If receive start email input prompt
											if (uart.find(responseBuffer))
											{
												// Remaining data consists of email input instruction 
												do
												{
													// We are running at the speed of light!
													while (!uart.available());
													// Read until end of line is found
													rxByte = uart.read();
												} while (rxByte != '\n');
											
												// Email header
												uart.print(F("From: "));
												uart.println(username);
												uart.print(F("To: "));
												uart.println(recipient);
												uart.print(F("Subject: "));
												uart.println(title);
												uart.print(F("\r\n"));
												// Email message
												uart.print(content);
												uart.print(F("\r\n.\r\n"));
												
												readFlash(emailSent, responseBuffer);
												
												// Email successfully sent
												if (uart.find(responseBuffer))
												{
													// Remaining data consists of incomprehensible 
													// email sent ID
													do
													{
														// We are running at the speed of light!
														while (!uart.available());
														// Read until end of line is found
														rxByte = uart.read();
													} while (rxByte != '\n');
													
													// WISMO228 gets exhausted after sending an email
													// Give him a short break
													delay(1000);
													// Revert to AT command mode
													uart.print(F("+++"));
													
													readFlash(ok, responseBuffer);
													
													// Data mode exited successfully
													if (uart.find(responseBuffer))
													{
														// Close the TCP socket
														uart.println(F("AT+WIPCLOSE=2,1"));
										
														if (uart.find(responseBuffer))
														{
															success = true;
														}	
													}
												}
											}
										}
									}	
								}
							}	
						}	
					}
				}
			}
		}
	}
	
	return (success);	
}


/*******************************************************************************
* Name: getClock
* Description: Retrieve the WISMO228 module clock.
*
* Argument  			Description
* =========  			===========
* 1. clock     		Clock in YY/MM/DD,HH:MM:SS*ZZ format
*									ZZ represent time difference between local & GMT time 
*									expressed in quarters of an hour. *ZZ has a range of 
*									-48 to +48. 
*									Example: 11/11/16,00:56:24+32 (GMT+8 local time).
*	
* Return					Description
* =========				===========
* 1. success 			True if clock is retrieved successfully or false if otherwise. 
*
*******************************************************************************/
bool WISMO228::getClock(char *clock)
{
	bool success = false;
	unsigned	char	clockCount;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	// If WISMO228 is on
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
	
		// Request current clock
		uart.println(F("AT+CCLK?"));
		
		readFlash(clockOk, responseBuffer);
		
		if (uart.find(responseBuffer))
		{
			if (waitForReply(CLOCK_COUNT_MAX, MIN_TIMEOUT))
			{
				for (clockCount = CLOCK_COUNT_MAX; clockCount > 0; clockCount--)
				{
					// Retrieve clock
					*clock++ = uart.read();
				}
				
				// Terminate the clock string
				*clock = '\0';
				
				// Expecting an "OK" response
				readFlash(ok, responseBuffer);
				
				if (uart.find(responseBuffer))
				{
					success = true;
				}
			}
		}
	}
	
	return (success);
}	

/*******************************************************************************
* Name: setClock
* Description: Set the WISMO228 module clock.
*
* Argument  			Description
* =========  			===========
* 1. clock     		Clock in YY/MM/DD,HH:MM:SS*ZZ format
*									ZZ represent time difference between local & GMT time 
*									expressed in quarters of an hour. *ZZ has a range of 
*									-48 to +48. 
*									Example: 11/11/16,00:56:24+32 (GMT+8 local time).
*	
* Return					Description
* =========				===========
* 1. success 			True if clock is set successfully or false if otherwise. 
*
*******************************************************************************/
bool WISMO228::setClock(const char *clock)
{
	bool success = false;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		
		// Check for correct clock string length
		if (strlen(clock) == CLOCK_COUNT_MAX)
		{
			uart.print(F("AT+CCLK=\""));
			uart.print(clock);
			uart.print(F("\"\r\n"));
			
			// Expecting an "OK" response
			readFlash(ok, responseBuffer);
			
			if (uart.find(responseBuffer))
			{
				success = true;
			}
		}
	}
	
	return (success);
}	

/*******************************************************************************
* Name: getRssi
* Description: Retrieve the WISMO228 module RSSI value in dBm.
*
* Argument  			Description
* =========  			===========
* 1. NIL
*	
* Return					Description
* =========				===========
* 1. rssi 				RSSI value in dBm. 
*
*******************************************************************************/
int	WISMO228::getRssi()
{
	int rssi;
	char	rxByte;
	
	rssi = 0;
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		uart.println(F("AT+CSQ"));
		
		readFlash(rssiCheck, responseBuffer);
		
		if (uart.find(responseBuffer))
		{
			while (!uart.available());
			
			rxByte = (char)uart.read();
			
			// Ensure numeric characters is received
			if ((rxByte >= '0') && (rxByte <= '9'))
			{
				// Convert ASCII 0-9 to integer
				rssi = rxByte - 0x30;
				
				while (!uart.available());
				
				rxByte = uart.read();
				
				if ((rxByte >= '0') && (rxByte <= '9'))
				{
					// Convert to tens
					rssi *= 10;
					// Add LSB
					rssi += rxByte - 0x30;
					
					// If RSSI value falls within range
					if ((rssi >= 0) && (rssi <= 31))
					{
						// Convert RSSI to dbm
						rssi = rssiToDbm(rssi);
					}
					
					while (!uart.available());
					
					rxByte = uart.read();
					
					if (rxByte == ',')
					{
						while (!uart.available());
						
						rxByte = uart.read();
						
						// Expecting an "OK" response
						readFlash(ok, responseBuffer);
						
						switch (rxByte)
						{
							case '0':
							case '1':
							case '2':
							case '3':
							case '4':
							case '5':
							case '6':
							case '7':
								// Check ending sequence
								if (!uart.find(responseBuffer))
								{
									rssi = 0;
								}
								break;
								
							case '9':
								while (!uart.available());
								rxByte = uart.read();
								// BER: 99
								if (rxByte == '9')
								{
									// Check ending sequence
									if (!uart.find(responseBuffer))
									{
										rssi = 0;
									}
								}
								break;
								
							default:
								rssi = 0;
								break;
						}
					}	
				}
				else if (rxByte == ',')
				{
					rssi = rssiToDbm(rssi);
				}
			}
		}
	}

	return (rssi);
}

/*******************************************************************************
* Name: rssiToDbm
* Description: Convert RSSI value into dBm.
*
* Argument  			Description
* =========  			===========
* 1. rssi					RSSI value in decimal 0-31.		
*	
* Return					Description
* =========				===========
* 1. rssi 				RSSI value in dBm. 
*
*******************************************************************************/
int	WISMO228::rssiToDbm(int	rssi)
{
	rssi = (rssi*2) + MINIMUM_SIGNAL_DBM;

	return (rssi);				
}

/*******************************************************************************
* Name: waitForReply
* Description: Wait for certain number of charaters from WISMO228 module within 
*							 a certain time frame.
*
* Argument  			Description
* =========  			===========
* 1. count				Number of expected characters from WISMO228 module.				
*
*	2. period				Time frame to wait for the reply from WISMO228 module in ms.
*
* Return					Description
* =========				===========
* 1. success			True if the expected number of characters from the WISMO228 
*									module is received within the stipulated time frame or false 
*									if otherwise.
*
*******************************************************************************/
bool	WISMO228::waitForReply(unsigned char count, long period)
{
	bool	success = false;
	unsigned long	timeout;
	
	timeout = millis() + period;
	
	while ((uart.available() < count) && (timeout > millis()));
	
	if ((uart.available() >= count) && (timeout > millis()))
	{
		success = true;
	}
	
	return (success);
}

/*******************************************************************************
* Name: openPort
* Description: Open a port on a server.
*
* Argument  			Description
* =========  			===========
* 1. server				Server name or IP.				
*
*	2. port					Port number in string format.
*
* Return					Description
* =========				===========
* 1. success			True if port is successfully open on the server or false if
*									otherwise.
*
*******************************************************************************/
bool	WISMO228::openPort(const char	*server, const char *port)
{
	bool	success = false;
	unsigned	char	attempt;
	
	// It takes more time for server to response to port open request	
	uart.setTimeout(MED_TIMEOUT);
	
	// Expecting port open OK response	
	readFlash(portOk, responseBuffer);	
	
	// Maximum 3 attempt to open a port with remote server
	for (attempt = 3; attempt > 0; attempt--)
	{
		// Create a TCP client socket with server with desired port number
		uart.print(F("AT+WIPCREATE=2,1,\""));
		uart.print(server);
		uart.print(F("\","));
		uart.println(port);
		
		if (uart.find(responseBuffer))
		{
			// Port is open
			success = true;
			break;
		}
	}
	
	// Revert back to minimal response time
	uart.setTimeout(MIN_TIMEOUT);
	
	return (success);
}

/*******************************************************************************
* Name: exchangeData
* Description: Initiate exchange of data process.
*
* Argument  			Description
* =========  			===========
* 1. NIL
*
* Return					Description
* =========				===========
* 1. success			True if data exchange procecss is successfully started or 
*									false if otherwise.
*
*******************************************************************************/
bool	WISMO228::exchangeData()
{
	bool	success = false;

	// Revert to medium response time	
	uart.setTimeout(MED_TIMEOUT);
	
	// Expecting data exchanging connection OK response
	readFlash(connectOk, responseBuffer);	
	// Initiate data exchange
	uart.println(F("AT+WIPDATA=2,1,1"));
		
	// If data exchanging is ready
	if (uart.find(responseBuffer))
	{
		success = true;
	}
	
	// Revert to minimum response time	
	uart.setTimeout(MIN_TIMEOUT);
	
	return (success);
}

/*******************************************************************************
* Name: encodeBase64
* Description: Base 64 encoder.
*
* Argument  			Description
* =========  			===========
* 1. input				String of characters to be encoded into base 64 format.
*
*	2. output				String of encoded characters in base 64 format. 
*
* Return					Description
* =========				===========
* 1. NIL
*
*******************************************************************************/
void	WISMO228::encodeBase64(const char *input, char *output)
{
	unsigned	int	length;
	char	raw[3];
	char	encoded[4];
	
	// Get input string length
	length = strlen(input);
	
	// Perform normal encoding for length more than 2 characters			
	while (length > 2)
	{
		// Retrieve 3 bytes at a time
		raw[0] = *input++;	
		raw[1] = *input++;
		raw[2] = *input++;
		
		// Reduce 3 characters
		length -= 3;
		
		// Expand 3 characters to 4 characters (8-bit each to 6-bit each)
		encoded[0] = (raw[0] & B11111100) >> 2;
		encoded[1] = ((raw[0] & B00000011) << 4) | ((raw[1] & B11110000) >> 4);
		encoded[2] = ((raw[1] & B00001111) << 2) | ((raw[2] & B11000000) >> 6);
		encoded[3] = (raw[2] & B00111111);
		
		// Retrieve corresponding encoded characters
		*output++ = pgm_read_byte_near(base64Table + encoded[0]);
		*output++ = pgm_read_byte_near(base64Table + encoded[1]);
		*output++ = pgm_read_byte_near(base64Table + encoded[2]);
		*output++ = pgm_read_byte_near(base64Table + encoded[3]);
	}
	
	// Special treatment for remaining 2 characters
	if (length == 2)
	{
		// Retrieve the last 2 characters and pad with 1 zero character
		raw[0] = *input++;
		raw[1] = *input;
    raw[2] = 0;
		
		encoded[0] = (raw[0] & B11111100) >> 2;
		encoded[1] = ((raw[0] & B00000011) << 4) | ((raw[1] & B11110000) >> 4);
		encoded[2] = ((raw[1] & B00001111) << 2) | ((raw[2] & B11000000) >> 6);	

		// Retrieve corresponding encoded characters and '=' for last character
		*output++ = pgm_read_byte_near(base64Table + encoded[0]);
		*output++ = pgm_read_byte_near(base64Table + encoded[1]);
		*output++ = pgm_read_byte_near(base64Table + encoded[2]);
		*output++ = '=';
	}
	
	// Special treatment for remaining 1 characters
	if (length == 1)
	{
		// Retrieve the last character and pad with 1 zero character
		raw[0] = *input;
    raw[1] = 0;
		
		encoded[0] = (raw[0] & B11111100) >> 2;
		encoded[1] = ((raw[0] & B00000011) << 4) | ((raw[1] & B11110000) >> 4);
		
		// Retrieve corresponding encoded characters and '=' for last 2 characters
		*output++ = pgm_read_byte_near(base64Table + encoded[0]);
		*output++ = pgm_read_byte_near(base64Table + encoded[1]);
		*output++ = '=';
		*output++ = '=';
	}
  // Terminate the output string
  *output = '\0';
}

/*******************************************************************************
* Name: readFlash
* Description: Retrieve string of characters from flash memory.
*
* Argument  			Description
* =========  			===========
* 1. sourcePtr		String of characters stored in flash memory.
*
*	2. targetPtr  	Location to store retrieved string in RAM. 
*
* Return					Description
* =========				===========
* 1. NIL
*
*******************************************************************************/
void WISMO228::readFlash(char *sourcePtr, char *targetPtr)
{
  // Read from flash until end of string 
  while (true)
  {
    // Retrieve a byte from flash
    *targetPtr = pgm_read_byte(sourcePtr++);
  
    // If end of string is found
    if (*targetPtr == '\0')
    {
      // All string characters retrieved
      break;
    }
    else
    {
      // Move on to the next character
      targetPtr++;
    }
  }
}