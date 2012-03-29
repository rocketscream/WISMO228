#ifndef WISMO228_h
#define WISMO228_h
#include	<avr/pgmspace.h>
#include	<SoftwareSerial.h>

#define NC	0xFF
#define	BAUD_RATE	9600
#define	REPLY_TIMEOUT 3000
#define	PING_TIMEOUT	5000
#define	SMS_TX_TIMEOUT	5000
#define NETWORK_SEARCH_DELAY 10000
#define STARTUP_DELAY 10000
#define	MINIMUM_SIGNAL_DBM	-113
#define	CLOCK_COUNT_MAX 20
#define	SMS_LENGTH_MAX	160
#define	RESPONSE_TIME_MAX	6

enum status_t{ 
	OFF, 
	ON, 
	GPRS_ON, 
	ERROR
};

class WISMO228
{
	public:
	
		WISMO228(unsigned char rxPin, unsigned char txPin, unsigned char onOffPin);	
		WISMO228(unsigned char rxPin, unsigned char txPin, unsigned char onOffPin, 
						 unsigned char ringPin, void (*newSmsFunction)(void));	
				 
		void	init();
		void	shutdown();
		bool	powerUp();
		
		bool	sendSms(const char *recipient, const char *message);
		bool	readSms(char *sender, char *message);
		
		bool	openGPRS(const char *apn, const char *username, const char *password);
		bool	closeGPRS();
		
		bool	getHttp(const char *server, const char *path, const char *port, 
									char *message, unsigned int limit);
		
		bool	sendEmail(const char *smtpServer, const char	*port, 
										const char *username, const char *password, 
										const char *recipient, const char *title, 
										const char *content);
		
		bool	getClock(char *clock);
		bool	setClock(char *clock);
		
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
		bool	replyCheck(const char *expected, long period);
		int	  rssiToDbm(int	rssi);
		void	encodeBase64(const char *input, char *output);
		
		SoftwareSerial	uart;
		void	(*functionPtr)(void);
		unsigned char	_onOffPin;
		unsigned char	_ringPin;
		status_t	status;
};


#endif