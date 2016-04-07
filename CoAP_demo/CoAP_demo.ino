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

//  Sample arduino program to demonstrate usage of coap-packet and coap-protocol

#include <ESP8266WiFi.h>
#include <coap-packet.h>
#include <coap-protocol.h>


const char*     ssid     = "your_ssid";
const char*     password = "your_password";

uns8            tokens[3] = {0x12, 0x22, 0x32};
uns16           messageID = 2000;

CoapPacket packet;
CoapProtocol protocol;

void setup() {
  Serial.begin(115200);
  connectWiFi();

  protocol.begin();
  protocol.setDestination("ip_address", 5683);
  protocol.setHandlers(packetAvailable, confirmSuccess, confirmFailed, responseTimedOut);

  makeOutgoingCONPacket();

  //Insert paccket we made to the queue
  protocol.addToTX(packet.getPacket(), packet.getPacketLength());
}

void loop() {
  protocol.process_rx_queue();
  protocol.process_tx_queue();
}

void makeOutgoingCONPacket() {
  packet.begin();   //Must always be called when starting a new packet
  packet.addHeader(TYPE_CON, COAP_GET, messageID++);
  packet.addTokens(3, &tokens[0]);
  packet.addOption(OPT_URI_PATH, 4, "demo");
  packet.addOption(OPT_URI_QUERY, 6, "test=1");
  packet.addPayload(13, "payload text");
  packet.printPacket();
  packet.readPacket();
}


void connectWiFi() {
  Serial.print("\nConnecting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IPv4: ");
  Serial.println(WiFi.localIP());
}

/////////////////////////////////////
//        Callback Functions      ///
/////////////////////////////////////

//Called whenever an unprocessed packet is found in RX
void packetAvailable(uns8* pkt, int pktLen) {
  packet.begin();
  packet.copyPacket(pkt, pktLen);
  Serial.println("New packet available");
  packet.readPacket();
  protocol.packetProcessed(packet.getID()); //We are done with this packet. Remove from queue
}

//Called whenever an outgoing CON packet has been re-submitted 4x and not acknowledged
void confirmFailed(uns8* pkt, int pktLen) {
  packet.begin();
  Serial.println("Did not get an ACK on time to this packet");
  packet.copyPacket(pkt, pktLen);
  packet.readPacket();
}

//Called whenever outgoing CON packet has been confirmed
void confirmSuccess(uns8* pkt, int pktLen) {
  packet.begin();
  packet.copyPacket(pkt, pktLen);
  Serial.println("ACK received for our CON packet");
  packet.readPacket();
}

void responseTimedOut(uns8* pkt, int pktLen) {
  packet.begin();
  packet.copyPacket(pkt, pktLen);
  Serial.println("Did not respond on time to this packet");
  packet.readPacket();
}



