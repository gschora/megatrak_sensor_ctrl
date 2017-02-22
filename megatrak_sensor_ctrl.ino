#include <RHDatagram.h>
#include <SPI.h>
#include <RH_RF69.h>

// Singleton instance of the radio driver
RH_RF69 rf69(15, 14);

#define SERVER_ADDRESS	1

// Class to manage message delivery and receipt, using the driver declared above
RHDatagram manager(rf69, SERVER_ADDRESS);

void setup() {

	Serial.begin(9600);
	while (!Serial && (millis() <= 8000));
	Serial.print("rfm69 server startup nr: ");
	Serial.println(SERVER_ADDRESS);

	if (!manager.init())
		Serial.println("init rfm69 failed");
	// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
	// No encryption
	if (!rf69.setFrequency(868.0))
		Serial.println("setFrequency failed");

	// If you are using a high power RF69, you *must* set a Tx power in the
	// range 14 to 20 like this:
	rf69.setTxPower(14);

}

uint8_t data[] = "dere zruck!";
uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];

void loop() {

	if (manager.available()) {

		// Should be a message for us now   
		uint8_t len = sizeof(buf);
		uint8_t from;
		if (manager.recvfrom(buf, &len, &from)) {

			Serial.print("got request from: ");
			Serial.print(from, DEC);
			Serial.print(": ");

			// trick zum zusammenbauen von low und High byte um int zu erhalten
			// siehe unter http://forum.arduino.cc/index.php?topic=99527.msg746371#msg746371
			uint16_t u_Int = *(uint16_t*)buf;
			Serial.println(u_Int);

			// Send a reply back to the originator client
      		if (!manager.sendto(data, sizeof(data), from))
        		Serial.println("sendto failed");
		}
		else {
			Serial.println("recv failed");
		}
	}
}
