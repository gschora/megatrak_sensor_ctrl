#include <RHDatagram.h>
#include <SPI.h>
#include <RH_RF69.h>
#include <EEPROM.h>
#include <SerialCommand.h>

#define EEPROM_NODE_ADDRESS 0
#define EEPROM_SERVER_ADDRESS 1
uint8_t SERVER_ADDRESS = EEPROM.read(EEPROM_SERVER_ADDRESS);
uint8_t NODE_ADDRESS = EEPROM.read(EEPROM_NODE_ADDRESS);

SerialCommand cmd;

RH_RF69 rf69(15, 14);
RHDatagram manager(rf69, NODE_ADDRESS);
uint8_t msg[RH_RF69_MAX_MESSAGE_LEN];

uint8_t involtPin[14] = {}; //equals involt.pin.P in app
String involtString[2] = {}; //equals involt.pin.S in app
char involt[16];
String involt_fname;

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
	involtReceive();
	involt_fname = "";
}

void chkMsg() {
	if (manager.available()) {
		// Should be a message for us now   
		uint8_t len = sizeof(msg);
		uint8_t from;
		if (manager.recvfrom(msg, &len, &from)) {
			if (msg[0] == 'u') {
				Serial.print("umin from: ");
				Serial.print(from, DEC);
				Serial.print(": ");
				//Baut den int aus high und lowbyte wieder zusammen
				uint16_t umin = ((int)msg[2] << 8) + msg[1];
				Serial.println(umin);
			}
			// trick zum zusammenbauen von low und High byte um int zu erhalten
			// siehe unter http://forum.arduino.cc/index.php?topic=99527.msg746371#msg746371
			//uint16_t u_Int = *(uint16_t*)msg;
		}
		else {
			Serial.println("recv failed");
		}
	}
}

void sendMsg(char* data, uint8_t rcvr) {
	// Send a reply back to the originator client
	if (!manager.sendto((uint8_t*)data, strlen(data), rcvr)) {
		Serial.println("sendto failed");
	}
	manager.waitPacketSent(); //wichtig!!!
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

//###################################################
void involtReceive() {
	if (Serial.available() > 0) {
		Serial.readBytesUntil('\n', involt, sizeof(involt));
		int pin;
		if (involt[0] == 'P') {
			int value;
			sscanf(involt, "P%dV%d", &pin, &value);
			involtPin[pin] = value;
		}
		else if (involt[0] == 'S') {
			char value[sizeof(involt)];
			sscanf(involt, "S%dV%s", &pin, &value);
			involtString[pin] = value;
		}
		else if (involt[0] == 'F') {
			char value[sizeof(involt)];
			sscanf(involt, "F%s", &value);
			involt_fname = value;
		};
		memset(involt, 0, sizeof(involt));
	};
};

void involtSend(int pinNumber, int sendValue) {
	Serial.print('A');
	Serial.print(pinNumber);
	Serial.print('V');
	Serial.print(sendValue);
	Serial.println('E');
	Serial.flush();
};

void involtSendString(int pinNumber, String sendString) {
	Serial.print('A');
	Serial.print(pinNumber);
	Serial.print('V');
	Serial.print(sendString);
	Serial.println('E');
	Serial.flush();

};

void involtSendFunction(String functionName) {
	Serial.print('F');
	Serial.print(functionName);
	Serial.println('E');
	Serial.flush();
};