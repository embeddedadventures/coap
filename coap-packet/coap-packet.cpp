#include "coap-packet.h"

/*		Helper functions	*/
inline uns8 invalid_option_num(uns8 optNum) {
	switch(optNum) {
		case OPTION_REPEAT:
		case OPT_IF_MATCH:
		case OPT_URI_HOST:
		case OPT_ETAG:
		case OPT_IF_NMATCH:
		case OPT_URI_PORT:
		case OPT_LOC_PATH:
		case OPT_URI_PATH:
		case OPT_CONTENT_FORMAT:
		case OPT_MAX_AGE:
		case OPT_URI_QUERY:
		case OPT_ACCEPT:
		case OPT_LOC_QUERY:
		case OPT_PROXY_URI:
		case OPT_PROXY_SCH:
		case OPT_SIZE1:
			return 1;
		default:
			return 0;
	}
}

void CoapPacket::CoapPacket() {}

void CoapPacket::~CoapPacket() {delete[] packet_buffer;}

/*		Packet Creation Functions	*/
void CoapPacket::begin() {
	pkt_length = 0;
	pkt_cursor = 0;
	token_length = 0;
	coap_code = 0;
	coap_type = 0;
	coap_msg_id = 0;
	num_options = 0;
	
	tkn_ptr = NULL;
	option_ptr = NULL;
	payload_ptr = NULL;
}

uns8 CoapPacket::addHeader(uns8 type, uns8 code, uns16 msg_id) {
	pkt_cursor = 0;
	coap_type = type;
	coap_code = code;
	coap_msg_id = msg_id;
	token_length = 0;
	pkt_buffer[pkt_cursor++] = ((COAP_VERSION << 6)  | (coap_type << 4)) & 0xF0;
	pkt_buffer[pkt_cursor++] = coap_code;
	pkt_buffer[pkt_cursor++] = coap_msg_id >> 8;
	pkt_buffer[pkt_cursor++] = coap_msg_id & 0xFF;
	
	pkt_length = pkt_cursor;	//Should be 4
	
	return 1;
}

uns8 CoapPacket::addTokens(uns8 tknLen, uns8* tknValue) {
	if (tknLen > 8)
		return 0;
	if (tknLen == 0)
		return 1;
	
	//Get token length, add to header
	token_length = tknLen;
	pkt_buffer[0] |= (token_length & 0x0F);
	
	//Add tokens
	tkn_ptr = &pkt_buffer[4];
	for (uns8 i = 0; i < token_length; i++) {
		pkt_buffer[pkt_cursor++] = *tknValue++;
	}
	pkt_length = pkt_cursor;
	return 1;
}

uns8 CoapPacket::addOption(uns8 optNum, uns16 optLen, const char *optParam) {
	//First check to make sure it's an actual option number
	if (invalid_option_num(optNum))
		return 0;
	
	
}

uns8 CoapPacket::addPayload(uns16 payloadLen, uns8 *payloadValue) {
	if (payloadLen == 0)
		return 1;
	
	pkt_buffer[pkt_cursor++] = PAYLOAD_MARK;
	payload_ptr = &pkt_buffer[pkt_cursor];	//Pointer will point to first byte of payload, NOT marker
	
	while ((*payloadValue) && (pkt_cursor < MAX_SIZE)) {
		pkt_buffer[pkt_cursor++] = *payloadValue++;
	}
	if (pkt_cursor >= MAX_SIZE)
		return 0;
	
	pkt_length = pkt_cursor;
	return 1;
}

uns16 CoapPacket::copy(uns8 *pktPtr, uns16 pktLen) {
	pkt_length = pktLen;
	pkt_cursor = 0;
	token_length = 0;
	coap_code = 0;
	coap_type = 0;
	coap_msg_id = 0;
	
	tkn_ptr = NULL;
	option_ptr = NULL;
	payload_ptr = NULL;
	
	while (*pktPtr) {
		pkt_buffer[pkt_cursor++] = *pktPtr++;
	}	
}

/*		Packet Information Functions	*/
uns16 CoapPacket::size() {
	return pkt_length;
}

uns8 CoapPacket::code() {
	return coap_code;
}

uns8 CoapPacket::type() {
	return coap_type;
}

uns16 CoapPacket::messageId() {
	return coap_msg_id;
}

/*		Packet Pointer Functions	*/
uns8* CoapPacket::packetPtr() {
	return &(this->pkt_buffer[0]);
}

uns8* CoapPacket::getTokenPtr() {
	return tkn_ptr;
}

uns8* CoapPacket::getPayloadPtr() {
	return payload_ptr;
}






