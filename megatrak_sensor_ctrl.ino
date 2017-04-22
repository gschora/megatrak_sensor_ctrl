#include <RHDatagram.h>
#include <SPI.h>
#include <RH_RF69.h>
#include <EEPROM.h>
#include <SerialCommand.h>

#define RESTART_ADDR       0xE000ED0C
#define READ_RESTART()     (*(volatile uint32_t *)RESTART_ADDR)
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

#define EEPROM_NODE_ADDRESS 0
#define EEPROM_SERVER_ADDRESS 1
uint8_t SERVER_ADDRESS = EEPROM.read(EEPROM_SERVER_ADDRESS);
uint8_t NODE_ADDRESS = EEPROM.read(EEPROM_NODE_ADDRESS);

SerialCommand cmd;

RH_RF69 rf69(15, 14);
RHDatagram manager(rf69, NODE_ADDRESS);
uint8_t msg[RH_RF69_MAX_MESSAGE_LEN];

struct SPEED{
	uint8_t nodeid;
	char type;
	uint8_t revNr;
	union {
		float f;
		unsigned char b[4];
	} val;
};
struct SPEED speed;

void setup() {
	Serial.begin(57600);
	while (!Serial && (millis() <= 8000));
	Serial.print("rfm69 server startup nr: ");
	Serial.println(SERVER_ADDRESS);

	//###################################################
	cmd.addCommand("sna", sc_setNodeAddress); //set node address
	cmd.addCommand("ssa", sc_setServerAddress); //set server address
	cmd.addCommand("cfg", sc_printCfg);
	cmd.addCommand("scn", sc_sndCmdNode); //send a command to a node over rf
	cmd.setDefaultHandler(sc_default);
	//###################################################

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

void loop() {
	cmd.readSerial();
	chkMsg();
	//involtReceive();
	//involt_fname = "";
}

void chkMsg() {
	if (manager.available()) {
		// Should be a message for us now   
		uint8_t len = sizeof(msg);
		uint8_t from;
		
		if (manager.recvfrom(msg, &len, &from)) {
			if (msg[0] == 'i') {
				Serial.println("from: ");
				Serial.print(from);
				Serial.print(" ");
				Serial.println((char*)msg);
			}
			else if (msg[0] == 'u' || msg[0] == 's') {
				decodeSpeed(msg,from);

			}
			//Serial.print("size ");
			//Serial.println(len);
			//Serial.println(msg[0]);
			//Serial.println(msg[5]);
			//Serial.println(msg[10]);
			//if (msg[0] == 'u') {
			//	//Serial.print("umin from: ");
			//	//Serial.print(from, DEC);
			//	//Serial.print(": ");
			//	//Baut den int aus high und lowbyte wieder zusammen
			//	uint16_t umin = ((int)msg[2] << 8) + (int)msg[1];
			//	//Serial.println(umin);
			//}
			//else if (msg[0] == 'i') {
			//	Serial.println("from: ");
			//	Serial.print(from);
			//	Serial.print(" ");
			//	Serial.println((char*)msg);
			//}
			// trick zum zusammenbauen von low und High byte um int zu erhalten
			// siehe unter http://forum.arduino.cc/index.php?topic=99527.msg746371#msg746371
			//uint16_t u_Int = *(uint16_t*)msg;
		}
		else {
			Serial.println("recv failed");
		}
		//memset(msg, 0, sizeof(msg)); //clearing the msg array IMPORTANT!!!!! otherwise old values from previous messages stay in there!!!

	}
}

void sendMsg(char* data, uint8_t rcvr) {
	// Send a reply back to the originator client
	if (!manager.sendto((uint8_t*)data, strlen(data), rcvr)) {
		Serial.println("sendto failed");
	}
	manager.waitPacketSent(); //wichtig!!!
}

void decodeSpeed(uint8_t* msg, uint8_t from) {
	speed.nodeid = from;
	speed.revNr = msg[1]; // -'0' ist trick um aus ascii-zahl einen int zu machen
	speed.type = msg[0];
	speed.val.b[0] = msg[2];
	speed.val.b[1] = msg[3];
	speed.val.b[2] = msg[4];
	speed.val.b[3] = msg[5];

	printSpeed();
}

void printSpeed() {
	Serial.print(speed.nodeid);
	Serial.print(":");
	Serial.print(speed.revNr);
	Serial.print(":");
	Serial.println(speed.val.f);
	//Serial.print(" ");
	//Serial.println(translateType(speed));
}

char* translateType(SPEED s) {
	switch (s.type) {
	case 'u':
		return "u/min";
		break;
	case 's':
		return "km/h";
	}
}


//###################################################
void sc_sndCmdNode() {
	uint8_t node_adress;
	char *ncmd;
	char *arg;

	arg = cmd.next();
	if (arg != NULL) {
		node_adress = atoi(arg);    // Converts a char string to an integer
	}
	else {
		Serial.println("scn - node address is missing! aborting...");
		return;
	}
	arg = cmd.next();
	if (arg != NULL) {
		ncmd = arg;
	}
	else {
		Serial.println("scn - node cmd is missing! aborting...");
		return;
	}

	sendMsg(ncmd, node_adress);

	Serial.print("rf command is to node: ");
	Serial.print(node_adress);
	Serial.print(" cmd: ");
	Serial.println(ncmd);
}
void setEEPROMNodeAddress(uint8_t address) {
	Serial.print("setting node address from ");
	Serial.print(EEPROM.read(EEPROM_NODE_ADDRESS), DEC);
	Serial.print(" to ");
	Serial.println(address);
	EEPROM.write(EEPROM_NODE_ADDRESS, address);
	Serial.println("done!");
	WRITE_RESTART(0x5FA0004);
}
uint8_t getEEPROMNodeAddress() {
	return EEPROM.read(EEPROM_NODE_ADDRESS);
}
void sc_setNodeAddress() {
	uint8_t address;
	char *arg;
	arg = cmd.next();
	if (arg != NULL) {
		address = atoi(arg);    // Converts a char string to an integer
	}
	else {
		Serial.println("sna - address is missing! aborting...");
		return;
	}
	setEEPROMNodeAddress(address);
}

void setEEPROMServerAddress(uint8_t address) {
	Serial.print("setting server address from ");
	Serial.print(EEPROM.read(EEPROM_SERVER_ADDRESS), DEC);
	Serial.print(" to ");
	Serial.println(address);
	EEPROM.write(EEPROM_SERVER_ADDRESS, address);
	Serial.println("done!");
}

uint8_t getEEPROMServerAddress() {
	return EEPROM.read(EEPROM_SERVER_ADDRESS);
}

void sc_setServerAddress() {
	uint8_t address;
	char *arg;
	arg = cmd.next();
	if (arg != NULL) {
		address = atoi(arg);    // Converts a char string to an integer
	}
	else {
		Serial.println("ssa - address is missing! aborting...");
		return;
	}
	setEEPROMServerAddress(address);
}

void sc_printCfg() {
	Serial.print("node address: ");
	Serial.println(getEEPROMNodeAddress());
	Serial.print("server address: ");
	Serial.println(getEEPROMServerAddress());
}

void sc_default(const char *command) {
	Serial.println("command not found! try:");
	Serial.println("ssa ... set server adress");
	Serial.println("sna ... set this nodes adress");
	Serial.println("scn ... send command to node");


}

