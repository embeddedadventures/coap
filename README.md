# CoAP - Constrained Application Protocol 
The Constrained Application Protocol is a communication protocol designed for resource-constrained devices. It gives a simple request/response interaction, can be easily translated to HTTP, and has very little overhead. More information can be read about CoAP <a href="https://tools.ietf.org/html/rfc7252">here</a>.

These Arduino libraries were designed to run a simplified CoAP client/server on the ESP8266 using our <a href="http://www.embeddedadventures.com/esp8266_wifi_module_wrl-esp12e.html">WRL-ESP12</a> or <a href="http://www.embeddedadventures.com/esp8266_wifi_module_wrl-esp7.html">WRL-ESP7</a>

## coap-protocol library

Using the coap-protocol library is very simple. Aside from creating a CoapPacket instance and a CoapProtocol instance, the main program needs to make 3 calls during setup and 2 calls during the loop. Everything else is taken care of. Four additional functions are necessary if the main program wants to handle events and process packets. 

1. On Setup...
  * *begin()* - initializes all the necessary data and pointers in the object
  * *setDestination(const char* ip, int portNum)* - set the ip address and port that the CoapProtocol object will communicate with
  * *setHandlers(...)*  - set all the handler functions that the CoapProtocol object will call on certain events
2. On loop...
  * *process_rx_queue()* & *process_tx_queue()* to handle the CoapProtocol's packet buffers
  * any packets or events that require the main program's attention will be made known through the handlers
3. Events that trigger a callback
  * new packet arrived -> *availablePacketHandler*
  * outgoing CONFIRMABLE packet successful -> *txSuccessHandler*
    * the main program's outgoing CONFIRMABLE packet received an ACK before MAJOR_TIMEOUT passed
  * outgoing CONFIRMABLE packet failed -> *txFailureHandler*
    * the main program's outgoing CONFIRMABLE packet doesn't receive an ACK packet and the MAJOR_TIMEOUT has passed
  * a CONFIRMABLE packet received and failed to respond on time -> *responseTimeoutHandler*
    * the main program received a CONFIRMABLE packet and failed to respond with an ACK packet on time. At this point, the CoapProtocol object will create an empty ACK packet and respond automatically.
  * each callback function passes a pointer to the packet and its length. The main program must copy the packet contents to its own CoapPacket object in order to handle the contents outside of the CoapProtocol.
  * once the main program is done with a packet (no longer needed), call *packetProcessed(uns16 id)* and pass the packet's message ID to remove it from the queue

**Example**
```
CoapPacket packet;
CoapProtocol protocol;

const char* ipaddress = "outgoing ip address";
int         portNumber = 1000;

void setup() {
  protocol.begin();
  protocol.setDestination(ipaddress, portNumber);
  protocol.setHandlers(packetAvailable, conSuccess, conFailed, responseTooLate);
  //Whatever else you need to setup 
  ....
}

void loop() {
  protocol.process_rx_queue();
  protocol.process_tx_queue();
  //Anything else that needs to be done here
  ...
}

//Function to handle a new packet received.
void packetAvailable(uns8* pkt, int pktLen) {
  packet.begin();
  packet.copyPacket(pkt, pktLen);
  packet.readPacket();  //Print out packet contents to Serial monitor
  protocol.packetProcessed(packet.getID());
  ...
}

//Function to handle a successful outgoing CON packet
void conSuccess(uns8* pkt, int pktLen) {...}

//Function to handle a failed outgoing CON packet
void conFailed(uns8* pkt, int pktLen) {...}

//Function to handle a received CON packet that main program didn't ACK on time
void responseTooLate(uns8* pkt, int pktLen) {...}
```



