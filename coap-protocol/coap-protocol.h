/*
Copyright (c) 2016, Embedded Adventures
All rights reserved.
Contact us at source [at] embeddedadventures.com
www.embeddedadventures.com
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
- Neither the name of Embedded Adventures nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
THE POSSIBILITY OF SUCH DAMAGE.
*/

// CoAP packet protocol handler, Arduino library
// Written originally by Embedded Adventures

#ifndef __COAP_PROTOCOL_h
#define __COAP-PROTOCOL_h

#include "ESP8266WiFi.h"
#include "WiFiUdp.h"
#include "coap-packet.h"

#define		uns32				unsigned long
#define		MAX_QUEUE_SIZE		4
#define		ACK_TIMEOUT			2
#define		ACK_RANDOM_FACTOR	1.5
#define		MAX_RETRANSMIT		4
#define		MAJOR_TIMEOUT		(ACK_TIMEOUT * (pow(2, MAX_RETRANSMIT) - 1) * ACK_RANDOM_FACTOR)

/*STATUS BYTE
	7		6		5 		4		3		2		1		0
 FILLED  IS_CON  ACK_SENT ACK_RCV PROCESS | num. times sent/rcvd
*/
#define		FLAG_FILLED			7		//0bF-------
#define		FLAG_IS_CON			6		//0b-C------
#define		FLAG_ACK_SENT		5		//0b--A-----
#define		FLAG_ACK_RCVD		4		//0b---A----
#define		FLAG_PROCESSED		3		//0b----P---
#define		COUNT_TRANSMISSIONS	0x07	//0b-----NNN

#define		RX					1
#define		TX					0

typedef void (*txStatus_callback)(uns16 messageID);
typedef void (*packetReturn_callback)(uns8* packet, int packetLength);


class CoapProtocol : public WiFiUDP {
	
private:
	const int		thisPort = 5683;
	int				portRemote;
	const char*		ipAddr;
	bool 			received;

	//Used for functions that can manipulate either rx or tx variables
	CoapPacket*		ptr_queue;
	uns8*			ptr_packetStatus;
	uns32*			timePtr;
	
	//RX variables
	CoapPacket		rxBuffer[MAX_QUEUE_SIZE];
	uns8			rxPacketStatus[MAX_QUEUE_SIZE];	
	uns32			rxTimeLog[MAX_QUEUE_SIZE];
	
	//TX variables
	CoapPacket		txBuffer[MAX_QUEUE_SIZE];
	uns8			txPacketStatus[MAX_QUEUE_SIZE];
	uns32			txTimeLog[MAX_QUEUE_SIZE];
	
	//Status checking
	inline int		numTimesTransmitted(uns8& stat);
	
	//Status and Overhead Query functions
	int		numEmptySpaces(int queue);	//1 = rx, 0 = tx
	int		numFilledSpaces(int queue); //1 = rx, 0 = tx
	int		findSpace(int queue);		//Returns MAX_QUEUE_SIZE if no space
	uns32	timeSent(int index);
	uns32	timeReceived(int index);
	int		timeExpired(int queue, int index);
	int		responseTimeExpired(int queue, int index);
	uns8	getPacketStatus(int queue, int index);
	
	//Callback functions
	packetReturn_callback	_txSuccess;
	packetReturn_callback 	_txFailure;
	packetReturn_callback 	_packetAvailable;
	packetReturn_callback 	_responseTimeout;
	
public:
	CoapProtocol();
	~CoapProtocol();
	
	//Set callback functions
	void	setSuccessCallback(packetReturn_callback txSuccess);
	void	setFailureCallback(packetReturn_callback txFailure);
	void	setAvailablePacketCallback(packetReturn_callback packetAvailable);
	void	setTimeoutCallback(packetReturn_callback responseTimeout);
	void	setHandlers(packetReturn_callback packetAvailable, 
							packetReturn_callback txSuccess,
							packetReturn_callback txFailure,
							packetReturn_callback responseTimeout);
	
	//Callback function handlers
	virtual void	txSuccessHandler(uns8* pkt, int pktLen);
	virtual void	txFailureHandler(uns8* pkt, int pktLen);
	virtual void	availablePacketHandler(uns8* pkt, int pktLen);
	virtual void	responseTimeoutHandler(uns8* pkt, int pktLen);
	
	//Other stuff
	int		begin();
	void	setDestination(IPAddress ip, int portNum);
	void	setDestination(const char* ip, int portNum);	
	
	//Packet functions
	uns8*	getPacket(int queue, int index);
	int		getPacketLength(int queue, int index);
	int		printPacket(int queue, int index);
		
	//RX & TX Buffer functions
	void	packetProcessed(uns16 id);
	void	clearQueue(int queue, int index = MAX_QUEUE_SIZE);
	void	process_rx_queue();	
	void	process_tx_queue();	
	int		addToTX(uns8 *packet, int len);
	
	//UDP Functions
	int		parseUDPPacket();
	int		receivePacket();
	int		sendPacket(int index);
	
	//Replying Functions
	int		addPayload(int index, int len, const char *pay);
	int		addTokens(int index, int numTokens, uns8 *tokens);
	int		addHeader(int index, uns8 type, uns8 code, uns16 id);
	int		emptyACK(int index);
};

#endif