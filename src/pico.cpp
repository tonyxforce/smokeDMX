#ifdef PICO

#include "DmxInput.h"
DmxInput dmxInput;

#define START_CHANNEL 1
#define NUM_CHANNELS 4

#define rxPin 3
#define smokePin 4
#define RED 5
#define GREEN 6
#define BLUE 7
#define fanPin 8

volatile uint8_t data[DMXINPUT_BUFFER_SIZE(startAddress, NUM_CHANNELS)];

void setup()
{
	// Setup our DMX Input to read on GPIO 2, from channel 1 to 3
	dmxInput.begin(rxPin, startAddress, NUM_CHANNELS);
	dmxInput.read_async(data);

	// Setup the onboard LED so that we can blink when we receives packets
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(smokePin, OUTPUT);
	pinMode(fanPin, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BLUE, OUTPUT);
	pinMode(RED, OUTPUT);
	digitalWrite(fanPin, !0);
	digitalWrite(smokePin, !0);
	digitalWrite(GREEN, 0);
	digitalWrite(BLUE, 0);
	digitalWrite(RED, 0);
	delay(500);
	digitalWrite(RED, 1);
	delay(500);
	digitalWrite(fanPin, !1);
	digitalWrite(smokePin, !0);
	digitalWrite(RED, 0);
	digitalWrite(GREEN, 1);
	delay(500);
	digitalWrite(fanPin, !1);
	digitalWrite(GREEN, !0);
	digitalWrite(BLUE, 1);
	delay(500);
	digitalWrite(fanPin, !0);
	digitalWrite(smokePin, !1);
	digitalWrite(BLUE, 0);
	delay(500);
	delay(500);
	digitalWrite(fanPin, !0);
	digitalWrite(smokePin, !0);
}

void loop()
{

	if (millis() - dmxInput.latest_packet_timestamp() > 100)
	{
		Serial.println("no data!");
		digitalWrite(LED_BUILTIN, 0);
		digitalWrite(smokePin, !0);
		digitalWrite(fanPin, !0);
		analogWrite(RED, 10);
		analogWrite(GREEN, 0);
		analogWrite(BLUE, 0);

		return;
	}
	// Print the DMX channels
	Serial.print("Received packet: ");
	for (uint i = 0; i < sizeof(data); i++)
	{
		Serial.print(data[i]);
		Serial.print(", ");
	}
	Serial.println("");
	digitalWrite(LED_BUILTIN, ISBETWEEN(100, data[startAddress + 1], 200));
	digitalWrite(smokePin, !ISBETWEEN(100, data[startAddress + 1], 200));
	digitalWrite(fanPin, !ISBETWEEN(100, data[startAddress + 2], 200));
	analogWrite(RED, data[startAddress + 2]);
	analogWrite(GREEN, data[startAddress + 3]);
	analogWrite(BLUE, data[startAddress + 4]);
}
#endif