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
sgn16ERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
THE POSSIBILITY OF SUCH DAMAGE.
*/

// CoAP packet builder, Arduino library
// Written originally by Embedded Adventures


#ifndef __COAP-PACKET_h
#define __COAP-PACKET_h

#define		uns8			uint8_t
#define		uns16			uint16_t
#define		sgn16			int16_t
#define		uns32			int32_t

#define 	MAX_SIZE		1250
#define		MAX_TOKENSIZE	8

#define		COAP_VERSION	0x01
#define 	PAYLOAD_MARK	0xFF
#define		MAX_OPTIONS		100

//Transaction types
#define TYPE_CON			0x00
#define TYPE_NON			0x01
#define TYPE_ACK			0x02
#define TYPE_RST			0x03

//Method codes
#define COAP_PING			0x00
#define COAP_GET			0x01
#define	COAP_POST			0x02
#define	COAP_PUT			0x03
#define	COAP_DEL			0x04

//Response codes - successes
#define	CODE_CREATED		0x41
#define CODE_DELETED		0x42
#define CODE_VALID			0x43
#define CODE_CHANGED		0x44
#define CODE_CONTENT		0x45
//Response codes - errors
#define CODE_BAD_REQUEST	0x80
#define CODE_UNAUTHORIZED	0x81
#define CODE_BAD_OPTION		0x82
#define CODE_FORBIDDEN		0x83
#define CODE_NOT_FOUND		0x84
#define CODE_NOT_ALLOWED	0x85
#define CODE_NOT_ACCEPTABLE	0x86
#define CODE_PRECOND_FAIL	0x8C
#define CODE_REQ_TOO_LARGE	0x8D
#define CODE_FORMAT_UNSUPP	0x8F
//Response codes - server errors
#define	CODE_sgn16_SERVER_ERR	0xA0
#define CODE_NOT_IMPLEM		0xA1
#define CODE_BAD_GATE		0xA2
#define CODE_SVC_UNAVAIL	0xA3
#define CODE_GATE_TIMEOUT	0xA4
#define CODE_PROXY_NSUPPORT 0xA5

//Option types
#define	OPTION_REPEAT		0
#define OPT_IF_MATCH		1
#define OPT_URI_HOST		3
#define OPT_ETAG			4
#define OPT_IF_NMATCH		5
#define OPT_URI_PORT		7
#define OPT_LOC_PATH		8
#define OPT_URI_PATH		11
#define OPT_CONTENT_FORMAT	12
#define OPT_MAX_AGE			14
#define OPT_URI_QUERY		15
#define OPT_ACCEPT			17
#define OPT_LOC_QUERY		20
#define OPT_PROXY_URI		35
#define OPT_PROXY_SCH		39
#define OPT_SIZE1			60

/*typedef struct {
	uns8		optionNumbers[MAX_OPTIONS];		//Stores option number
	uns8		*paramAddr[MAX_OPTIONS];		//Stores address of first byte of parameter
	sgn16		paramLength[MAX_OPTIONS];		//Stores length of param
	sgn16		cursor;
}	coap_options_struct;*/


//This struct is for 1 option
typedef struct {
	uns8	*option_ptr;
	uns8	option_number;
	uns16	option_length;
	uns8	*option_value_ptr;
}	coap_option_struct;


class CoapPacket {
private:
	uns8	pkt_buffer[MAX_SIZE];
	sgn16	pkt_length;
	sgn16	pkt_cursor;

	uns8	token_length;
	uns8	coap_code;
	uns8	coap_type;
	uns16	coap_msg_id;
	uns8	num_options;
	
	uns8	*tkn_ptr;
	uns8	*option_ptr;
	uns8	*payload_ptr;
	
public:
	CoapPacket();
	~CoapPacket();
	
	void	begin();
	uns8	addHeader(uns8 type, uns8 code, uns16 msg_id);
	uns8	addTokens(uns8 tknLen, uns8 *tknValue);
	uns8	addOption(uns8 optNum, uns16 optLen, const char *optParam);
	uns8	addPayload(uns16 payloadLen, uns8 *payloadValue);
	
	uns16	copy(uns8 *pktPtr, uns16 pktLen);
	
	uns16	size();
	uns8	code();
	uns8	type();
	uns16	messageId();
	
	uns8*	packetPtr();
	uns8*	getTokenPtr();
	uns8*	getPayloadPtr();
	
	
	
	


};	


#endif
