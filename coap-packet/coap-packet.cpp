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

// CoAP packet builder, Arduino library
// Written originally by Embedded Adventures

#include "Arduino.h"
#include "coap-packet.h"

CoapPacket::CoapPacket() {}

CoapPacket::~CoapPacket() {}

/*	Resets all indices in packet. Should be called whenever a new packet is to be made	*/
int CoapPacket::begin() {
	//Reset all the indeces
	index = 0;
	hdrIndex = index;
	tknIndex = 0;
	payloadIndex = 0;
	optionIndex = 0;
	parser = 0;
	//Reset the counts
	lastOptionNumber = 0;
	numOptions = 0;
	//Reset the packetOptions, set the indices to -1
	packetOptions.cursor = 0;
	for (int i = 0; i < MAX_OPTIONS; i++) {
		packetOptions.paramAddr[i] = NULL;
		packetOptions.optionNumbers[i] = 0;
	}
	return 1;
}


/////////////////////////////////////////////////////
/////				Packet Building 			/////
/////////////////////////////////////////////////////

/*	Returns 0 if the index of the packet is past MAX_SIZE */
int CoapPacket::addHeader(uns8 type, uns8 code, uns16 id) {
	packetBuffer[index++] = (((VERSION) << 6) | ((type & 0x03) << 4));	// byte 0
	packetBuffer[index++] = code;	// byte 1
	packetBuffer[index++] = id >> 8;	// byte 2
	packetBuffer[index++] = id & 0xFF;	// byte 3
	return (index > MAX_SIZE) ? 0 : 1;	
}

/*	Returns 0 if token length > 8 or if index is past MAX_SIZE	*/
int CoapPacket::addTokens(int tokenLength, uns8 *tokenValue) {
	if (tokenLength >= MAX_TOKENSIZE)
		return 0;
	tknIndex = index;	
	packetBuffer[hdrIndex] = (packetBuffer[hdrIndex] & 0xF0) | (uns8)tokenLength;
	for (int i = 0; i < tokenLength; i++) {
		packetBuffer[index++] = *tokenValue++;
	}
	return (index > MAX_SIZE) ? 0 : 1;

}

/*	Returns 0 if payload already exists or if index > MAX_SIZE	*/
int CoapPacket::addPayload(int len, const char *payloadOut) {
	//payloadIndex should be 0 unless there was already payload content added before
	if (payloadIndex != 0)
		return 0;
	payloadIndex = index;
	packetBuffer[index++] = 0xFF;	// payload indicator
	for (int i = 0; i < len; i++) {
		packetBuffer[index++] = payloadOut[i];
	}
	packetLength = index;
	return (index > MAX_SIZE) ? 0 : 1;
	
}

/*	Returns 0 if index > MAX_SIZE. Options must be added in increasing order	*/
int CoapPacket::addOption(uns8 optNum, int len, const char *param) {
	optionIndex = index;	// where THIS option starts
	if (numOptions == 0)
		optionStartIndex = index;
	if (processDelta(optNum))	//Checks if the given option number satisfies the order of placement	
		processLength(len);		//If it does, option param length is processed
	else
		return 0;
	
	//At this point, the option is valid and we now add the params.
	//First, save the starting index of the param to our object
	for (int i = 0; i < len; i++) {
		packetBuffer[index++] = param[i];
		if (index > MAX_SIZE) {
			return 0;
		}
	}
	numOptions++;
	return (index > MAX_SIZE) ? 0 : 1;
}

/*
	Calculates the option delta value based on the current option number and the previous one
	Generates additional bytes for option deltas greater than 12
	Returns false if the option number given does not fit the increasing order requirement
*/
bool CoapPacket::processDelta(uns8 optNum) {
	if (lastOptionNumber > optNum)	// must be presented in increasing order
		return false;
	uns16 delta;
	delta = optNum - lastOptionNumber;
	lastOptionNumber = optNum;
	if (delta > 268) { 
		delta = delta - 269;
		packetBuffer[index++] = 14 << 4;	// 14 indicating a 16 bit number in optionIndex + 1
		packetBuffer[index++] = delta >> 8;
		packetBuffer[index++] = delta & 0xff;
	} else if (delta > 12) {
		delta = delta - 13;
		packetBuffer[index++] = 13 << 4;	// 13 indicating an 8 bit number in optionIndex + 1
		packetBuffer[index++] = delta;
	} else {  // option delta is 12 or less
		packetBuffer[index++] = delta << 4;
	}
	return true;
}

/*	Calculates the option length bytes	*/
void CoapPacket::processLength(int len) {
	if (len > 268) { // it's a big one
		len = len - 269;
		packetBuffer[optionIndex] = (packetBuffer[optionIndex] & 0xF0) | 14;
		packetBuffer[index++] = len >> 8;
		packetBuffer[index++] = len & 0xff;

	} else if (len > 12) {
		packetBuffer[optionIndex] = (packetBuffer[optionIndex] & 0xF0) | 13;
		packetBuffer[index++] = len - 13;
	}
	else {
		packetBuffer[optionIndex] = (packetBuffer[optionIndex] & 0xF0) | len;
	}
}

/*	Reads in a packet. Resulting packet will be unparsed -> MUST parse it after	*/
int	CoapPacket::copyPacket(uns8 *rx, int len) {
	if (!packetBuffer) 
		return 0;
	else {
		index = 0;
		for (int i = 0; i < len; i++) {
			packetBuffer[index++] = rx[i];
		}
		packetBuffer[index] = '\0';
		packetLength = index;
	}
	return 1;	
}

void CoapPacket::setIndex(int i) {
	this->index = i;
	this->packetLength = index;
}


/////////////////////////////////////////////////////
/////			Readable Coap Packet 			/////
/////////////////////////////////////////////////////

/*	Goes through packet and marks the indeces of the header, tokens, options, payload	*/
int CoapPacket::parsePacket() {
	int tokenLength = 0, lastDelta = 0, optionCursor = 0;
	parser = 0;
	optionIndex = 0;
	optionStartIndex = 0;
	payloadIndex = 0;
	tknIndex = 0;
	hdrIndex = 0;
	numOptions = 0;
	
	//Find out if there are tokens (length 1 - 8)
	if (((packetBuffer[0] & 0x0F) > 0) && ((packetBuffer[0] & 0x0F) < 9)) {
		tokenLength = packetBuffer[0] & 0x0F;
		tknIndex = 4;
	}
	parser = 4;
	parser += tokenLength;
	//Now, parser should be after tokens
	while (parser < packetLength) {
		if ((packetBuffer[parser] != PAYLOAD) && (packetBuffer[parser] != '\0')) {	//At this point there should be options
			if (optionStartIndex == 0)
				optionStartIndex = parser;
			uns8 optionHdr = packetBuffer[parser];
			uns16 delta = (optionHdr >> 4) & 0x000F;
			uns16 len = optionHdr & 0x000F;
			//Special cases for option deltas > 12
			if (delta == 13) {
				parser++;
				delta = packetBuffer[parser] + 13;
			}
			else if (delta == 14) {
				parser++;
				delta = ((packetBuffer[parser++] << 8) | (packetBuffer[parser])) + 269;
			}
			//Here we save the delta to our array
			packetOptions.optionNumbers[optionCursor] = delta;
			lastDelta += delta;
			delta = lastDelta;
			//Special cases for option lengths greater than 12
			if (len == 13) {
				parser++;
				len = packetBuffer[parser] + 13;
			}
			else if (len == 14) {
				parser++;
				len = ((packetBuffer[parser++] << 8) | (packetBuffer[parser])) + 269;
			}
			
			parser++;
			//At this point, parser should be on the first byte of the option param.
			//Add the index to packetOptions paramAddr
			packetOptions.paramAddr[optionCursor] = &packetBuffer[parser];
			packetOptions.paramLength[optionCursor] = len;
			optionCursor++;
			parser += len;
			numOptions++;
		}
		else {
			payloadIndex = parser;
			parser = packetLength;
		}
	}
	
	//Right now, option_struct has all deltas. We turn them into the right option no.
	for (int i = 1; i < numOptions; i++) {
		packetOptions.optionNumbers[i] += packetOptions.optionNumbers[i - 1];
	}
	return 1;
}

/*	Prints packet contents in hex	*/
int CoapPacket::printPacket() {
	if (!packetBuffer) {
		Serial.println("\n\tPACKET DOESN'T EXIST\n");
		return 0;
	}
	Serial.print("\n----PACKET----\n");

	for (int i = 0; i < index; i++) {
		Serial.print(packetBuffer[i], HEX);
		Serial.print(" ");
	}
	return 1;
}

/*	Print packet in human-readable form	*/
int CoapPacket::readPacket() {
	printHeader();
	printTokens();
	if (numOptions > 0)
		printOptions();
	else
		Serial.println("----NO OPTIONS----");
	printPayload();
	return 1;
}

void CoapPacket::printHeader() {
	uns8 vers, msgType, msgCode, tokenLength;
	uns16 msgID = ((packetBuffer[2] << 8) & 0xFF00) | (packetBuffer[3] & 0x00FF);
	vers = (packetBuffer[0] >> 6) & 0x03;
	msgType = (packetBuffer[0] >> 4) & 0x03;
	tokenLength = packetBuffer[0] & 0x0F;
	msgCode = packetBuffer[1];
	
	Serial.print("Version\t");
	Serial.println(vers, HEX);

	Serial.print("Message Type:\t");
	if (msgType == TYPE_CON)
		Serial.println("TYPE_CON");
	else if (msgType == TYPE_NON)
		Serial.println("TYPE_NON");
	else if (msgType == TYPE_ACK)
		Serial.println("TYPE_ACK");
	else if (msgType == TYPE_RST)
		Serial.println("TYPE_RST");
	else if (msgType == TYPE_RST)
		Serial.println("TYPE_RST");
	
	Serial.print("Token Length:\t");
	Serial.println(tokenLength);

	Serial.print("Response/Request Code:\t");
	Serial.println(msgCode);
	
	Serial.print("Message ID:\t");
	Serial.println(msgID);
}

void CoapPacket::printTokens() {
	int tokenLength = packetBuffer[0] & 0x0F;
	if (tokenLength > 0) {
		Serial.print("------TOKENS------\n");
		Serial.print("Token length:\t");
		Serial.println(tokenLength);
		for (int i = 0; i < tokenLength; i++) {
			Serial.print(packetBuffer[i + 4], HEX);
			Serial.print(" ");
		}
		Serial.println();
	}
	else
		Serial.print("\n----NO TOKENS----\n");
}

void CoapPacket::printOptions() {
	int index = optionStartIndex;
	int lastDelta = 0; 
	for (int i = 0; i < numOptions; i++) {
		uns8 optionHdr = packetBuffer[index];
		uns16 delta = (optionHdr >> 4) & 0x000F;
		uns16 len = optionHdr & 0x000F;
		if (delta == 13) {
			index++;
			delta = packetBuffer[index] + 13;
		}
		else if (delta == 14) {
			index++;
			delta = ((packetBuffer[index++] << 8) | (packetBuffer[index])) + 269;
		}
		lastDelta += delta;
		delta = lastDelta;
		
		if (len == 13) {
			index++;
			len = packetBuffer[index] + 13;
		}
		else if (len == 14) {
			index++;
			len = ((packetBuffer[index++] << 8) | (packetBuffer[index])) + 269;
		}
		Serial.print("\n------OPTION------\n");
		Serial.print("Option Number:\t");
		Serial.println(delta, DEC);
		Serial.print("Option Length:\t");
		Serial.println(len, DEC);
		Serial.print("Params:\t");
		index++;
		for (int i = 0; i < len; i++) {
			Serial.print((char)packetBuffer[index]);
			index++;
		}
		Serial.println();		
	}
}

void CoapPacket::printPayload() {
	if (payloadIndex > 0) {
		Serial.println("----PAYLOAD----");
		for (int i = payloadIndex; i < packetLength; i++) {
			Serial.print((char)packetBuffer[i]);
		}
		Serial.println();
	}
	else
		Serial.println("----NO PAYLOAD----");
}


/////////////////////////////////////////////////////
/////				Accessors					/////
/////////////////////////////////////////////////////

/*	Returns addresss of first byte in packet	*/
uns8* CoapPacket::getPacket() {
	if (!packetBuffer)
		return 0;
	return &packetBuffer[0];
}

/*	Returns packet length	*/
int CoapPacket::getPacketLength() {
	packetLength = index;
	return packetLength;
}

uns16 CoapPacket::getMessageType() {
	return ((packetBuffer[hdrIndex] >> 4) & 0x03);
}

uns8 CoapPacket::getResponseCode() {
	return packetBuffer[hdrIndex + 1];
}

uns8 CoapPacket::getTokenLength() {
	return (packetBuffer[hdrIndex] & 0x0F);
}

/*	Returns address of first token	*/
uns8* CoapPacket::getTokens() {
	int len = packetBuffer[hdrIndex] & 0x0F;
	if ((len == 0) || (len > 8))
		return 0;
	else
		return &packetBuffer[4];
}

uns16 CoapPacket::getID() {
	return ((uns16)(packetBuffer[2] << 8) | ((uns16)packetBuffer[3] & 0x00FF));
}

/*	Returns address of payload marker byte	*/
uns8* CoapPacket::getPayloadAddr() {
	if (packetBuffer[payloadIndex] != 0xFF)
		return NULL;
	return &packetBuffer[payloadIndex];
}

int CoapPacket::getPayloadIndex() {
	return payloadIndex;
}


/////////////////////////////////////////////////////
/////			Options Accessor				/////
/////////////////////////////////////////////////////

/*	Returns first option number. Usage of this must be followed by option param, ALWAYS	*/
int CoapPacket::optionStart() {
	packetOptions.cursor = 0;
	if (numOptions > 0)
		return packetOptions.optionNumbers[packetOptions.cursor];
	else
		return -1;
}

int CoapPacket::getNextOption() {
	if (packetOptions.cursor < numOptions)
		return packetOptions.optionNumbers[++packetOptions.cursor];
	else 
		return -1;
}

/*	Returns number of options	*/
int CoapPacket::getOptionCount() {
	return numOptions;
}

uns8* CoapPacket::getOptionParameter() {
	return packetOptions.paramAddr[packetOptions.cursor];
}

int CoapPacket::getParameterLength() {
	return packetOptions.paramLength[packetOptions.cursor];
}




