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

#include "Arduino.h"
#include "WiFiUdp.h"
#include "coap-protocol.h"

CoapProtocol::CoapProtocol() {}

CoapProtocol::~CoapProtocol() {}

int CoapProtocol::begin() {
	for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
		rxPacketStatus[i] = 0;	//Clear status bytes
		txPacketStatus[i] = 0;
		rxTimeLog[i] = 0;		//Clear the time log
		txTimeLog[i] = 0;
	}
	WiFiUDP::begin(thisPort);
	return 1;
}

void CoapProtocol::setDestination(const char* ip, int portNum) {
	ipAddr = ip;
	portRemote = portNum;
}

/*	Returns number of times the packet was transmitted */
int CoapProtocol::numTimesTransmitted(uns8& stat) {
	return (stat & COUNT_TRANSMISSIONS);
}


////////////////////////////////////////////////////
////		Status & Overhead Functions			////
////////////////////////////////////////////////////

/*	Returns number of empty spaces in the buffer QUEUE	*/
int CoapProtocol::numEmptySpaces(int queue) {
	uns8 empty = 0;
	if (queue)
		ptr_packetStatus = &rxPacketStatus[0];
	else
		ptr_packetStatus = &txPacketStatus[0];
	
	for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
		empty += bitRead(*ptr_packetStatus, FLAG_FILLED);
	}
	return empty;
}

/*	Returns number of filled spaces in the buffer QUEUE */
int CoapProtocol::numFilledSpaces(int queue) {
	uns8 filled = MAX_QUEUE_SIZE;
	if (queue)
		ptr_packetStatus = &rxPacketStatus[0];
	else
		ptr_packetStatus = &txPacketStatus[0];
	
	for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
		if (!bitRead(*ptr_packetStatus, FLAG_FILLED))
			filled--;
	}
	return filled;
}

/*	Returns index of first empty space tx/rx buffer. Returns 4 if full	*/
int CoapProtocol::findSpace(int queue) {
	if (queue)
		ptr_packetStatus = &rxPacketStatus[0];
	else
		ptr_packetStatus = &txPacketStatus[0];
	
	int cursor = 0;
	while ((bitRead(ptr_packetStatus[cursor], FLAG_FILLED)) && (cursor < MAX_QUEUE_SIZE)) {
		cursor++;
	}
	return cursor;
}

/*	Returns the time packet at index INDEX was sent	*/
uns32 CoapProtocol::timeSent(int index) {
	return txTimeLog[index];
}

/*	Returns the time packet at index INDEX was received	*/
uns32 CoapProtocol::timeReceived(int index) {
	return rxTimeLog[index];
}

/*	Checks if TIMEOUT has elapsed since the packet was logged in
	-1 if empty
	0 if TIMEOUT hasn't passed
	1 if TIMEOUT has passed
*/
int CoapProtocol::timeExpired(int queue, int index) {
	//Change pointers to point to either RX vars or TX vars
	if (queue) {
		ptr_packetStatus = &rxPacketStatus[0];
		timePtr = &rxTimeLog[0];
	} else {
		ptr_packetStatus = &txPacketStatus[0];
		timePtr = &txTimeLog[0];
	}

	if (!bitRead(ptr_packetStatus[index], 7))
		return -1;
	//If TIMEOUT has passed since packet sent/rcvd
	else if ((millis() - timePtr[index]) > (MAJOR_TIMEOUT * 1000)) 
		return 1;
	//If TIMEOUT not passed yet, return 0
	else 
		return 0;							
}

int CoapProtocol::responseTimeExpired(int queue, int index) {
	//Change pointers to point to either RX vars or TX vars
	if (queue) {
		ptr_packetStatus = &rxPacketStatus[index];
		timePtr = &rxTimeLog[index];
	} else {
		ptr_packetStatus = &txPacketStatus[index];
		timePtr = &txTimeLog[index];
	}
	
	uns8 transmitted = *ptr_packetStatus & COUNT_TRANSMISSIONS;	//We need to know how many times it's been transmitted
	long timeout = pow(ACK_TIMEOUT, transmitted) * 1000;
	
	if (!bitRead(ptr_packetStatus[index], FLAG_FILLED))	//If empty, return -1
		return -1;
	else if ((millis() - timePtr[index]) > timeout) 	//If LATE_RESPONSE has passed since packet sent/rcvd
		return 1;
	else 
		return 0;							//If LATE_RESPONSE not passed yet, return 0
}

/*	Returns packet status byte of packet INDEX in buffer QUEUE	*/
uns8 CoapProtocol::getPacketStatus(int queue, int index) {
	if (queue)
		return rxPacketStatus[index];
	else
		return txPacketStatus[index];
}

//Marks as processed the packet in rxQueue with same id
void CoapProtocol::packetProcessed(uns16 id) {
	for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
		if ((bitRead(rxPacketStatus[i], FLAG_FILLED)) && (rxBuffer[i].getID() == id))
			bitSet(rxPacketStatus[i], FLAG_PROCESSED);
	}
}


////////////////////////////////////////////////////
////			Packet Functions				////
////////////////////////////////////////////////////

/*	Returns address of first byte of packet in INDEX in buffer QUEUE	*/
uns8* CoapProtocol::getPacket(int queue, int index) {
	if (queue)
		return rxBuffer[index].getPacket();
	else
		return txBuffer[index].getPacket();
}

/*	Returns length of packet in index INDEX at buffer QUEUE	*/
int CoapProtocol::getPacketLength(int queue, int index) {
	if (queue)
		return rxBuffer[index].getPacketLength();
	else
		return txBuffer[index].getPacketLength();
}

/*	Prints packet in index INDEX in buffer QUEUE in a readable manner, or return -1 if empty	*/
int CoapProtocol::printPacket(int queue, int index) {
	if (queue) {
		ptr_packetStatus = &rxPacketStatus[index];
		ptr_queue = &rxBuffer[index];
	} else {
		ptr_packetStatus = &txPacketStatus[index];
		ptr_queue = &txBuffer[index];
	}
	if (!bitRead(ptr_packetStatus[index], 7))
		return -1;
	ptr_queue[index].readPacket();
	return 1;	
}


////////////////////////////////////////////////////
////				Set Callbacks				////
////////////////////////////////////////////////////

void CoapProtocol::setSuccessCallback(packetReturn_callback txSuccess) {
	_txSuccess = txSuccess;
}

void CoapProtocol::setFailureCallback(packetReturn_callback txFailure) {
	_txFailure = txFailure;
}

void CoapProtocol::setAvailablePacketCallback(packetReturn_callback packetAvailable) {
	_packetAvailable = packetAvailable;
}

void CoapProtocol::setTimeoutCallback(packetReturn_callback responseTimeout) {
	_responseTimeout = responseTimeout;
}

void CoapProtocol::setHandlers(packetReturn_callback packetAvailable, 
							packetReturn_callback txSuccess,
							packetReturn_callback txFailure,
							packetReturn_callback responseTimeout) 
	{
	_txSuccess = txSuccess;
	_txFailure = txFailure;
	_packetAvailable = packetAvailable;
	_responseTimeout = responseTimeout;
}


////////////////////////////////////////////////////
////				 Callbacks					////
////////////////////////////////////////////////////

void CoapProtocol::txSuccessHandler(uns8* pkt, int pktLen) {
	if (_txSuccess != NULL) {
		_txSuccess(pkt, pktLen);
	}
}

void CoapProtocol::txFailureHandler(uns8* pkt, int pktLen) {
	if (_txSuccess != NULL) {
		_txFailure(pkt, pktLen);
	}
}

void CoapProtocol::availablePacketHandler(uns8* pkt, int pktLen) {
	if (_txSuccess != NULL) {
		_packetAvailable(pkt, pktLen);
	}
}

void CoapProtocol::responseTimeoutHandler(uns8* pkt, int pktLen) {
	if (_txSuccess != NULL) {
		_responseTimeout(pkt, pktLen);
	}
}


////////////////////////////////////////////////////
////			Buffer Functions				////
////////////////////////////////////////////////////

/*	EMPTY -> go to next packet
//	PROCESSED? -> remove from queue, go to next packet
//	ACK? -> find matching CON in tx
//				match found -> txSuccess(ID), remove both from queues
//				match not found -> remove from buffer
//	CON -> check time
//		time "expired?" -> responseTimeout(&packet), send empty ACK, remove from queue
//		time not "expired" -> packetReceived(&packet)	
//	RST/NON -> packetReceived(&packet)	
//	3 callback functions - txSuccess(ID), responseTimeout(&packet), packetAvailable(&packet)*/

void CoapProtocol::process_rx_queue() {
	for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
		//If packet index is empty, skip 
		if (!bitRead(rxPacketStatus[i], FLAG_FILLED))
			continue;
		
		//If packet is processed, remove from buffer go to next one
		else if (bitRead(rxPacketStatus[i], FLAG_PROCESSED)) {
			rxPacketStatus[i] = 0;
			rxTimeLog[i] = 0;
			continue;
		}
		
		//If packet is an ACK, find matching CON in TX
		if (bitRead(rxPacketStatus[i], FLAG_ACK_RCVD)) {
			int cursor = 0;
			//Look for matching CON in TX
			while ((cursor < MAX_QUEUE_SIZE) && bitRead(txPacketStatus[cursor], FLAG_FILLED)) {
				 if (rxBuffer[i].getID() == txBuffer[cursor].getID())
					 break;
				 else
					cursor++;
			}
			//This means matching CON was not found in tx. Remove
			if (cursor == MAX_QUEUE_SIZE) {
				rxPacketStatus[i] = 0x00;
				rxTimeLog[i] = 0;
			}
			//Matching CON has been found. Callback, then remove
			else {
				txSuccessHandler(rxBuffer[i].getPacket(), rxBuffer[i].getPacketLength());
				rxPacketStatus[i] = 0x00;
				rxTimeLog[i] = 0;
				txPacketStatus[cursor] = 0;
				txTimeLog[cursor] = 0;
			}
		}
		
		//Packet is CON -> check its time
		else if (bitRead(rxPacketStatus[i], FLAG_IS_CON)) {
			//Response time has expired
			if (responseTimeExpired(RX, i)) {
				int x = findSpace(TX);
				//If there's space in TX, send empty ACK and remove this one
				if (x < MAX_QUEUE_SIZE) {
					addHeader(x, TYPE_ACK, rxBuffer[i].getResponseCode(), rxBuffer[i].getID());
					sendPacket(x);
					rxPacketStatus[i] = 0;
					rxTimeLog[i] = 0;
					txPacketStatus[x] = 0;
					txTimeLog[x] = 0;
				}
				responseTimeoutHandler(rxBuffer[i].getPacket(), rxBuffer[i].getPacketLength());
			}
			else {
				availablePacketHandler(rxBuffer[i].getPacket(), rxBuffer[i].getPacketLength());
			}
		}
		
		//Packet is NON or RST
		else {
			//Insert packetAvailable(&packet) callback
			availablePacketHandler(rxBuffer[i].getPacket(), rxBuffer[i].getPacketLength());
		}
	}
}

/*	EMPTY -> go to next packet
//	NOT SENT? -> send, go to next packet
//	SENT & NON/RST? -> remove from queue
//	SENT & ACK -> remove from queue
//	SENT & CON -> check time
//			majorTimeout? -> txFailed(&packet), remove from queue
//			responseTimeout? -> re-send
//			no time has expired yet -> next packet;
//	1 callback function - txFailed(&packet)*/

void CoapProtocol::process_tx_queue() {
	for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
		//Packet index empty - next packet
		if (!bitRead(txPacketStatus[i], FLAG_FILLED)) {
			continue;
		}
		
		//Packet not sent yet - send
		else if ((txPacketStatus[i] & COUNT_TRANSMISSIONS) == 0) {
			sendPacket(i);
		}
		//Packet sent before - NON/RST/ACK type -> remove from queue
		if ((txBuffer[i].getMessageType() == TYPE_NON) || 
			(txBuffer[i].getMessageType() == TYPE_RST) ||
			(txBuffer[i].getMessageType() == TYPE_ACK)) {
			txPacketStatus[i] = 0;
			txTimeLog[i] = 0;
		}
		
		//Packet sent before - CON -> check time
		else {
			//Check major timeout
			if (timeExpired(TX, i)) {
				txFailureHandler(txBuffer[i].getPacket(), txBuffer[i].getPacketLength());
				txPacketStatus[i] = 0;
				txTimeLog[i] = 0;
			}
			
			//Major timeout not expired. Check response timeout
			else if (responseTimeExpired(TX, i)) {
				sendPacket(i);
			}
			
			//Packet sent more than 5x already, remove
			else if ((txPacketStatus[i] & COUNT_TRANSMISSIONS) == 5) {
				txFailureHandler(txBuffer[i].getPacket(), txBuffer[i].getPacketLength());
			}
			//Neither timeouts have expired - do nothing
		}
	}
}

/* 
Clears packet in buffer[queue], at slot [index] 
If index = 4, entire buffer is cleared. This is also default if no index is passed*/
void CoapProtocol::clearQueue(int queue, int index) {
	if (queue) {
		ptr_packetStatus = &rxPacketStatus[0];
		timePtr = &rxTimeLog[0];
	} else {
		ptr_packetStatus = &txPacketStatus[0];
		timePtr = &txTimeLog[0];
	}
	
	if (index == 4) {
		ptr_packetStatus[0] = 0;
		ptr_packetStatus[1] = 0;
		ptr_packetStatus[2] = 0;
		ptr_packetStatus[3] = 0;
		
		timePtr[0] = 0;
		timePtr[1] = 0;
		timePtr[2] = 0;
		timePtr[3] = 0;
	}
	else {
		ptr_packetStatus[index] = 0;
		timePtr[index] = 0;
	}
}

/*	Add packet to txQueue. Returns -1 if full	*/

int CoapProtocol::addToTX(uns8 *packet, int len) {
	int index = findSpace(TX);
	if (index == MAX_QUEUE_SIZE)
		return -1;
	txBuffer[index].begin();
	txBuffer[index].copyPacket(packet, len);
	
	//Set FILLED flag
	bitSet(txPacketStatus[index], FLAG_FILLED);	
	txBuffer[index].parsePacket();

	//Set CON flag if it's a CON packet
	if (txBuffer[index].getMessageType() == TYPE_CON) {
		bitSet(txPacketStatus[index], FLAG_IS_CON);
	}
	
	return 1;
}


//////////////////////
//	UDP Functions	//
//////////////////////

int CoapProtocol::sendPacket(int index) {
		WiFiUDP::beginPacket(ipAddr, portRemote);
		WiFiUDP::write(txBuffer[index].getPacket(), txBuffer[index].getPacketLength());
		WiFiUDP::endPacket();
		txTimeLog[index] = millis();	//Log time sent
		txPacketStatus[index]++;
		return index;
}

int CoapProtocol::parseUDPPacket() {
	return WiFiUDP::parsePacket();
}

/*	Receives incoming packet.
	places it in the buffer, parses it, and sets the appropriate flags.
	Returns the index of the packet or -1 if packet dropped.
	*/
int CoapProtocol::receivePacket() {
	//Find space in rx queue
	int index = findSpace(RX);	
	if (index == MAX_QUEUE_SIZE) {
		return -1;
	}
	rxBuffer[index].begin();
	int len = WiFiUDP::read(rxBuffer[index].getPacket(), MAX_SIZE);
	
	//Set the packet's index manually, since this doesn't use copyPacket()
	rxBuffer[index].setIndex(len);
	
	//Log time
	rxTimeLog[index] = millis();
	
	//Set FILLED flag
	bitSet(rxPacketStatus[index], FLAG_FILLED);
	
	//Increase rx count for this packet
	rxPacketStatus[index]++;	
	
	//Set the CON/ACK flags in packetStatus and queueStatus
	if (rxBuffer[index].getMessageType() == TYPE_CON) {
		bitSet(rxPacketStatus[index], FLAG_IS_CON);
	} 
	else if (rxBuffer[index].getMessageType() == TYPE_ACK) {
		bitSet(rxPacketStatus[index], FLAG_ACK_RCVD);
	}
	
	//Set the pointers to the parts of the packet
	rxBuffer[index].parsePacket();
	return 1;
}


//////////////////////////////////////////////
///			Replying Functions			   ///
//////////////////////////////////////////////

int CoapProtocol::addPayload(int index, int len, const char *pay) {
	return txBuffer[index].addPayload(len, pay);
}

int CoapProtocol::addTokens(int index, int numTokens, uns8 *tokens) {
	return txBuffer[index].addTokens(numTokens, tokens);
}

int CoapProtocol::addHeader(int index, uns8 type, uns8 code, uns16 id) {
	txBuffer[index].begin();
	return txBuffer[index].addHeader(type, code, id);
}

/*	
Create simple response packet. 
Returns -1 if txBuffer is full, otherwise it returns index
*/
int CoapProtocol::emptyACK(int index) {
	int x = findSpace(TX);	//Find first empty space in tx queue
	if (x == MAX_QUEUE_SIZE)
		return -1;
	txBuffer[x].begin();
	txBuffer[x].addHeader(TYPE_ACK, rxBuffer[index].getResponseCode(), rxBuffer[index].getID());
	bitSet(txPacketStatus[x], FLAG_FILLED);
	return x;
}



