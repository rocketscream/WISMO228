/*******************************************************************************
* WISMO228 Library
* Version: 1.00
* Date: 28-03-2012
* Company: Rocket Scream Electronics
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
* 1.00      Initial public release. Tested with L22 & L23 of WISMO228 firmware.
*						Only works with Arduino IDE 1.0.
*						HTTP POST method is not implemented yet.
*						All "find" function (part of Stream class) will be replaced 
*						with a similar function that all allows usage of string stored in 
*						flash instead in the next revision. 
*******************************************************************************/
// ***** INCLUDES *****
#include "WISMO228.h"
#include <Arduino.h>

prog_uchar	base64Table[] PROGMEM =	{"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
																			"abcdefghijklmnopqrstuvwxyz"
																			"0123456789+/"};

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
	unsigned long	timeout;
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
		
		// Wait for WISMO228 to get ready
		//while (digitalRead(A1) == LOW);
		
		timeout = millis() + STARTUP_DELAY;
		while (timeout > millis());
		
		// Remove power up invalid characters
		uart.flush();
		// Configure reply timeout period
		uart.setTimeout(REPLY_TIMEOUT);
		
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
		digitalWrite(_onOffPin, LOW);		// but 3000 ms is good enough

		uart.flush();
		uart.end();
		// WISMO228 is shutdown
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
	
	timeout = millis() + STARTUP_DELAY;
		
	// Wait for SIM card initialization
	while (timeout > millis())
	{
		uart.println(F("AT+CPIN?"));
		
		//if (replyCheck("\r\n+CPIN: READY\r\n\r\nOK\r\n\0", 
		// 							 REPLY_TIMEOUT))
		if (uart.find("\r\n+CPIN: READY\r\n\r\nOK\r\n\0"))
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
	char	rxByte;
	
	uart.println(F("ATE0"));
	
	if (waitForReply(1, REPLY_TIMEOUT))
	{
		rxByte = uart.peek();
		
		if (rxByte == 'A')
		{
			//if (replyCheck("ATE0\r\n\r\nOK\r\n\0", REPLY_TIMEOUT))
			if (uart.find("ATE0\r\n\r\nOK\r\n\0"))
			{	
				success = true;
			}
		}
		else
		{
			//if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
			if (uart.find("\r\nOK\r\n\0"))
			{	
				success = true;
			}
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
	
	timeout = millis() + NETWORK_SEARCH_DELAY;
		
	// Wait for network registeration
	while (timeout > millis())
	{
		uart.println(F("AT+CREG?"));
		
		//if (replyCheck("\r\n+CREG: 0,1\r\n\r\nOK\r\n\0", REPLY_TIMEOUT))
		if (uart.find("\r\n+CREG: 0,1\r\n\r\nOK\r\n\0"))
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
		
	//if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
	if (uart.find("\r\nOK\r\n\0"))
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
	
	//if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
	if (uart.find("\r\nOK\r\n\0"))
	{
		// Attach interrupt for RING pin as new SMS indicator
		attachInterrupt((_ringPin - 2), functionPtr, FALLING); 
		//digitalWrite(_ringPin, HIGH);
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
			uart.setTimeout(SMS_TX_TIMEOUT);
			
			// Wait for message writing prompt
			if (uart.find("\r\n> \0"))
			{
				uart.print(message);
				uart.write(26);
			
				if (uart.find("\r\n+CMGS: \0"))
				{
					// Revert back to normal reply timeout
					uart.setTimeout(REPLY_TIMEOUT);
					
					if (waitForReply(3, REPLY_TIMEOUT))
					{
						// Read out all SMS ID
						while ((char)uart.read() != '\r');
					
						if (uart.find("\n\r\nOK\r\n\0"))
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
	
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Retrieve unread SMS
		uart.println(F("AT+CMGL=\"REC UNREAD\""));
		
		if (uart.find("\r\n+CMGL: "))
		{
			// Maximum index is 255 (3 characters)
			// 4th character is ","
			if (waitForReply(4, REPLY_TIMEOUT))
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
				
				if (uart.find("\"REC UNREAD\",\"+\0"))
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
					
					if (uart.find(",\"\0"))
					{
						do
						{
							// We are running at lightning speed, wait for a while
							while (!uart.available());
							rxByte = uart.read();
						}
						while (rxByte != '"');
						
						if (uart.find(",\"\0"))
						{
							// We are running at ligthning speed, have to wait for data
							if (waitForReply(CLOCK_COUNT_MAX, REPLY_TIMEOUT))
							{
								// Retrieve time stamping 
								for (rxCount = CLOCK_COUNT_MAX; rxCount > 0; rxCount--)
								{
									// Can be used for time reference
									rxByte = uart.read();
								}

								if (uart.find("\"\r\n\0"))
								{
									if (waitForReply(1, REPLY_TIMEOUT))
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

										if (uart.find("\nOK\r\n\0"))
										{
											// Delete the SMS to avoid overflow
											uart.print(F("AT+CMGD="));
											uart.println(index);
								
											if (uart.find("\r\nOK\r\n\0"))
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
	
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Start TCP/IP stack
		uart.println(F("AT+WIPCFG=1"));
		
		if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
		{
			// Open GPRS bearer
			uart.println(F("AT+WIPBR=1,6"));
			
			if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
			{
				// Set the APN
				uart.print(F("AT+WIPBR=2,6,11,\""));
				uart.print(apn);
				uart.println(F("\""));
				
				if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
				{
					// Set the username
					uart.print(F("AT+WIPBR=2,6,0,\""));
					uart.print(username);
					uart.println(F("\""));
					
					if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
					{
						// Set the password
						uart.print(F("AT+WIPBR=2,6,11,\""));
						uart.print(password);
						uart.println(F("\""));
						
						if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
						{
							// Start GPRS bearer
							uart.println(F("AT+WIPBR=4,6,0"));
							
							if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
							{
								// GPRS connection is up
								status = GPRS_ON;
								success = true;
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
	
	// Only close GPRS connection if currently on
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Stop TCP/IP stack (automatically detach from GPRS)
		uart.println(F("AT+WIPCFG=0"));
		
		if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
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
	
	// Needs GPRS connection to execute ping
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		// Execute the PING service
		uart.print(F("AT+WIPPING=\""));
		uart.print(url);
		uart.println(F("\""));

		if (replyCheck("\r\nOK\r\n\r\n+WIPPING: \0", PING_TIMEOUT))
		{
			// Ping is successful
			if (replyCheck("0,0,\0", REPLY_TIMEOUT))
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
					
				if (replyCheck("\n\0", REPLY_TIMEOUT))
				{
					// Convert port number into string
					sscanf(responseTimeStr, "%u", &responseTime);
				}
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
				
				if (replyCheck("\r\n\0", REPLY_TIMEOUT))
				{
					if (waitForReply(1, REPLY_TIMEOUT))
					{
						rxByte = uart.read();
						
						// If <CR><LF>OK<CR><LF> is received
						if (rxByte == 'O')
						{
							if (replyCheck("K\r\n\0", REPLY_TIMEOUT))
							{
								// Close the TCP socket with server
								uart.println(F("AT+WIPCLOSE=2,1"));
					
								if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
								{
									success = true;
								}
							}
						}
						// Server closes session automatically
						if (rxByte == '+')
						{
							if (replyCheck("WIPPEERCLOSE: 2,1\r\n\r\nOK\r\n\0", REPLY_TIMEOUT))
							{
								// Close the TCP socket with server
								uart.println(F("AT+WIPCLOSE=2,1"));
					
								if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
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
	
	// Flush remaining unrecognized characters
	uart.flush();
	
	return (success);
}

/*******************************************************************************
* Name: postHttp
* Description: Perform HTTP method POST to retrieve or send data to a server.
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
bool	postHttp(char *server, char *path, unsigned int port, char *message)
{
	bool	success = false;
	
	// To be implemented
	
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

	
	// If currently attach to GPRS
	if (status == GPRS_ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		
		if (openPort(smtpServer, port))
		{
			if (replyCheck("\r\n+WIPDATA: 2,1,\0", 5000))
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
				
				if (replyCheck("\n\0", REPLY_TIMEOUT))
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
						
						// Retrieve subsequent response from server within a stipulated time
						//timeout = millis() + 5000;
						
						delay (5000);
						/*while (millis() < timeout)
						{
							while (!uart.available());
							rxByte = uart.read();
						}*/
						uart.flush();
				
						// Initiate account login
						uart.println(F("AUTH LOGIN"));
				
						// Receive the username prompt in base 64 format
						if (replyCheck("334 VXNlcm5hbWU6\r\n\0", REPLY_TIMEOUT))
						{
							// Encode username into base 64 format
							encodeBase64(username, base64);
							// Send username in base 64 format
							uart.println(base64);
							
							// Receive the password prompt in base 64 format
							if (replyCheck("334 UGFzc3dvcmQ6\r\n\0", REPLY_TIMEOUT))
							{
								// Encode username into base 64 format
								encodeBase64(password, base64);		
								// Send password in base 64 format
								uart.println(base64);
								
								if (replyCheck("235 Authentication succeeded\r\n\0", 
																REPLY_TIMEOUT))
								{
									// Email sender
									uart.print(F("MAIL FROM: "));
									uart.println(username);
									
									if (replyCheck("250 OK\r\n\0", REPLY_TIMEOUT))
									{
										// Email recipient
										uart.print(F("RCPT TO: "));
										uart.println(recipient);
										
										if (replyCheck("250 Accepted\r\n\0", REPLY_TIMEOUT))
										{
											// Start of email body
											uart.println(F("DATA"));
											
											// Receive start email input prompt
											if (replyCheck("354 \0", REPLY_TIMEOUT))
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
													
												// Email successfully sent
												if (replyCheck("250 OK \0", REPLY_TIMEOUT))
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
						
													// Data mode exited successfully
													if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
													{
														// Close the TCP socket
														uart.println(F("AT+WIPCLOSE=2,1"));
										
														if (replyCheck("\r\nOK\r\n\0", REPLY_TIMEOUT))
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
	
	// If WISMO228 is on
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
	
		// Request current clock
		uart.println(F("AT+CCLK?"));
		
		if (replyCheck("\r\n+CCLK: \"", REPLY_TIMEOUT))
		{
			if (waitForReply(CLOCK_COUNT_MAX, REPLY_TIMEOUT))
			{
				for (clockCount = CLOCK_COUNT_MAX; clockCount > 0; clockCount--)
				{
					// Retrieve clock
					*clock++ = uart.read();
				}
				
				// Terminate the clock string
				*clock = '\0';
				
				if (replyCheck("\"\r\n\r\nOK\r\n", REPLY_TIMEOUT))
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
bool WISMO228::setClock(char *clock)
{
	bool success = false;
	
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
			
			if (replyCheck("\r\nOK\r\n", REPLY_TIMEOUT))
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
	
	if (status == ON)
	{
		// Clear any unwanted data in UART
		uart.flush();
		uart.println(F("AT+CSQ"));
		
		if (replyCheck("\r\n+CSQ: \0", REPLY_TIMEOUT))
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
								if (!replyCheck("\r\n\r\nOK\r\n\0", REPLY_TIMEOUT))
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
									if (!replyCheck("\r\n\r\nOK\r\n\0", REPLY_TIMEOUT))
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
* Name: replyCheck
* Description: Check reply from WISMO228 within a certain time frame.
*
* Argument  			Description
* =========  			===========
* 1. expected			Expected response from WISMO228 module.				
*
*	2. period				Time frame to wait for the reply from WISMO228 module.
*
* Return					Description
* =========				===========
* 1. success			True if the response from the WISMO228 module is as expected
*									within the stipulated time frame or false if otherwise.
*
*******************************************************************************/
bool WISMO228::replyCheck(const char *expected, long period)
{
	unsigned char	rxCount;
	bool	success = false;
	unsigned long	timeout;
	
	// Maximum waiting time for response
	timeout = millis() + period;
	
	while (timeout > millis())
	{
		if (uart.available() >= (unsigned char)strlen(expected))
		{
			for (rxCount = strlen(expected); rxCount > 0; rxCount--)
			{
				if (uart.read() == *expected)
				{
					// Check next character
					expected++;
				}
				else
				{
					break;
				}
			}
			
			if (rxCount == 0)	success = true;
			break;
		}
	}
	
	return (success);
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
	
	// Create a TCP client socket with server with desired port number
	uart.print(F("AT+WIPCREATE=2,1,\""));
	uart.print(server);
	uart.print(F("\","));
	uart.println(port);
		
	if (replyCheck("\r\nOK\r\n\r\n+WIPREADY: 2,1\r\n\0", REPLY_TIMEOUT))
	{
		success = true;
	}
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
	
	// Initiate data exchange
	uart.println(F("AT+WIPDATA=2,1,1"));
			
	if (replyCheck("\r\nCONNECT\r\n\0", REPLY_TIMEOUT))
	{
		success = true;
	}
	
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