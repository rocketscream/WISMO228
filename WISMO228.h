#pragma once

#include "Arduino.h"

#define __PROG_TYPES_COMPAT__
#include	<avr/pgmspace.h>

#define NC	0xFF
#define	BAUD_RATE	9600
#define	MIN_TIMEOUT 3000
#define	MED_TIMEOUT	5000
#define MAX_TIMEOUT 10000
#define	MINIMUM_SIGNAL_DBM	-113
#define	CLOCK_COUNT_MAX 20
#define	SMS_LENGTH_MAX	160
#define	RESPONSE_TIME_MAX	6
#define	RESPONSE_LENGTH_MAX 30

enum status_t{ 
	OFF, 
	ON, 
	GPRS_ON, 
	ERROR
};

// ***** CONSTANTS *****
// ***** EXPECTED WISMO228 RESPONSE *****

static const char simOk [] PROGMEM = "\r\n+CPIN: READY\r\n\r\nOK\r\n";
static const char ok [] PROGMEM = "OK";
static const char networkOk [] PROGMEM = "\r\n+CREG: 0,1\r\n\r\nOK\r\n";
static const char smsCursor [] PROGMEM = "> ";
static const char smsSendOk [] PROGMEM = "\r\n+CMGS: ";
static const char smsList [] PROGMEM = "\r\n+CMGL: ";
static const char smsUnread [] PROGMEM = "\"REC UNREAD\",\"+";
static const char commaQuoteMark [] PROGMEM = ",\"";
static const char quoteMark [] PROGMEM = "\"";
static const char newLine [] PROGMEM = "\r\n";
static const char carriegeReturn [] PROGMEM = "\r";
static const char lineFeed [] PROGMEM = "\n";
static const char pingOk [] PROGMEM = "\r\nOK\r\n\r\n+WIPPING: 0,0,";
static const char clockOk [] PROGMEM = "\r\n+CCLK: \"";
static const char rssiCheck [] PROGMEM = "\r\n+CSQ: ";
static const char portOk [] PROGMEM = "+WIPREADY: 2,1\r\n";
static const char connectOk [] PROGMEM = "\r\nCONNECT\r\n";
static const char dataOk [] PROGMEM = "\r\n+WIPDATA: 2,1,";
static const char usernamePrompt [] PROGMEM = "334 VXNlcm5hbWU6\r\n";
static const char passwordPrompt [] PROGMEM = "334 UGFzc3dvcmQ6\r\n";
static const char authenticationOk [] PROGMEM = "235 Authentication succeeded\r\n";
static const char senderOk [] PROGMEM = "250 OK\r\n";
static const char recipientOk [] PROGMEM = "250 Accepted\r\n";
static const char emailInputPrompt [] PROGMEM = "354 ";
static const char emailSent [] PROGMEM = "250 OK ";
static const char shutdownLink [] PROGMEM = "SHUTDOWN";

// ***** BASE64 ENCODING TABLE *****
static const char	base64Table [] PROGMEM = { "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
											   "abcdefghijklmnopqrstuvwxyz"
											   "0123456789+/" };

// ***** VARIABLES *****
static char responseBuffer[RESPONSE_LENGTH_MAX];

template<class SerialPort>
class WISMO228
{
	public:
		WISMO228(SerialPort& serialPort, unsigned char onOffPin);
		WISMO228(SerialPort& serialPort, unsigned char onOffPin, unsigned char ringPin, void(*newSmsFunction)(void));
			 
		void	init();
		void	shutdown();
		bool	powerUp();
		
		bool	sendSms(const char *recipient, const char *message);
		bool	readSms(char *sender, char *message);
		
		bool	openGPRS(const char *apn, const char *username, const char *password);
		bool	closeGPRS();
		
		bool	getHttp(const char *server, const char *path, const char *port, 
									char *message, unsigned int limit);
		
		bool	putHttp(const char *server, const char *path, const char *port, 
                  const char *host, const char *data, const char *controlKey, 
									const char *contentType);					
		
		bool	sendEmail(const char *smtpServer, const char	*port, 
										const char *username, const char *password, 
									  const char *recipient, const char *title, 
										const char *content);
		
		bool	getClock(char *clock);
		bool	setClock(const char *clock);
		
		unsigned int	ping(const char	*url);
		
		status_t	getStatus();
		
		int	getRssi();
	
	private:
		bool	simReady();
		bool	offEcho();
		bool	registerNetwork();
		bool	textModeSms();
		bool	newSmsSetup();
		bool	openPort(const char	*server, const char *port);
		bool	exchangeData();
		bool	waitForReply(unsigned char count, long period);
		int	  rssiToDbm(int	rssi);
		void	encodeBase64(const char *input, char *output);
		void	readFlash(const char *sourcePtr, char *targetPtr);
		
		void	(*functionPtr)(void);
		unsigned char	_onOffPin;
		unsigned char	_ringPin;
		status_t	status;

		SerialPort& uart;
};

#include "WISMO228.hpp"

#define WISMO_CREATE_INSTANCE(SerialPort, Name, onOffPin) \
    WISMO228<typeof(SerialPort)> Name((typeof(SerialPort)&)SerialPort, onOffPin);

#if defined(ARDUINO_SAM_DUE) || defined(USBCON)
// Leonardo, Due and other USB boards use Serial1 by default.
#define WISMO_CREATE_DEFAULT_INSTANCE()                                      \
        WISMO_CREATE_INSTANCE(Serial1, wismo, A2);
#else
/*! \brief Create an instance of the library with default name, serial port
and settings, for compatibility with sketches written with pre-v4.2 MIDI Lib,
or if you don't bother using custom names, serial port or settings.
*/
#define WISMO_CREATE_DEFAULT_INSTANCE()                                      \
        WISMO_CREATE_INSTANCE(Serial,  wismo, A2);
#endif