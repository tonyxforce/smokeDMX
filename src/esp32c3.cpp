#include "main.h"
#ifdef ESP32C3

#include <Preferences.h>
Preferences preferences;

#if __has_include(<U8g2lib.h>)
#include <U8g2lib.h>
#else
#if __has_include(<u8g2lib.h>)
#include <u8g2lib.h>
#else
#error no u8g2lib
#endif
#endif

#include <Wire.h>

// ...and then set the communication pins!
const int tx_pin = 21;
const int rx_pin = 20;
const int rts_pin = 10;

const int smokePin = 0;

// ledc channels
const int RED = 0;
const int GREEN = 1;
const int BLUE = 2;

const int MODE = 4;
const int UP = 5;
const int DOWN = 6;
const int ENTER = 7;
const int buttonPins[4] = {MODE, UP, DOWN, ENTER};

const int SDApin = 8;
const int SCLpin = 9;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

#include "main.h"
#include "esp_dmx.h"
#include "rdm/responder.h"

dmx_packet_t packet;

const dmx_port_t dmxPort = DMX_NUM_0;

// First, use the default DMX configuration...
dmx_config_t config = {
		.interrupt_flags = (1 << 10),
		.root_device_parameter_count = 32,
		.sub_device_parameter_count = 0,
		.model_id = 0,
		.product_category = RDM_PRODUCT_CATEGORY_ATMOSPHERIC,
		.software_version_id = ESP_DMX_VERSION_ID,
		.software_version_label = ESP_DMX_VERSION_LABEL,
		.queue_size_max = 32};

int ledPin = 8;
void rdmIdentifyCallback(dmx_port_t dmxPort, rdm_header_t *request_header,
												 rdm_header_t *response_header, void *context)
{
	/* We should only turn the LED on and off when we send a SET response message.
	This prevents extra work from being done when a GET request is received. */
	if (request_header->cc == RDM_CC_SET_COMMAND)
	{
		bool identify;
		rdm_get_identify_device(dmxPort, &identify);
		digitalWrite(ledPin, !identify);
		Serial.printf("Identify mode is %s.\n", identify ? "on" : "off");
	}
}

// ...declare the driver's DMX personalities...
const int personality_count = 1;

uint8_t data[DMX_PACKET_SIZE];
uint8_t data0[DMX_PACKET_SIZE]; // Initialize with zeros

dmx_personality_t personalities[] = {
		{1, "Default Personality"}};

int remainingFrames = 2;

void printCenter(const char text[], int y)
{
	u8g2.drawStr((128 / 2) - (u8g2.getStrWidth(text) / 2), y, text);
};

bool buttonStatesBef[4];

const int freq = 100000;
const int resolution = 8;

void setup()
{
	Serial.begin(921600);
	Serial.println("Starting ArtNet DMX Receiver");
	pinMode(1, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(3, OUTPUT);
	digitalWrite(1, 0);
	digitalWrite(2, 0);
	digitalWrite(3, 0);

	Wire.begin(SDApin, SCLpin);
	if (!u8g2.begin())
	{
		Serial.println("Unsuccessful display begin");
	}
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_font_logisoso32_tf);
	printCenter("BOOT", 64 - 16);
	u8g2.sendBuffer();
	// delay(2000);
	u8g2.clearBuffer();
	Serial.println("Sent buffer");

	preferences.begin("espdmx", false);

	// ...install the DMX driver...
	dmx_driver_install(dmxPort, &config, personalities, personality_count);

	rdm_register_device_model_description(dmxPort, "RDM Test Fixture",
																				rdmIdentifyCallback, NULL);

	rdm_register_identify_device(dmxPort, rdmIdentifyCallback, NULL);

	// dmx_set_start_address(dmxPort, preferences.getUShort("addr", 0));

	/* 		pinMode(ledPin, OUTPUT);
			pinMode(smokePin, OUTPUT);
			digitalWrite(ledPin, HIGH); */

	for (int i = 0; i < 4; i++)
	{
		pinMode(buttonPins[i], INPUT_PULLUP);
		buttonStatesBef[i] = 0;
	}

	ledcSetup(0, freq, resolution);
	ledcAttachPin(2, RED);
	ledcSetup(1, freq, resolution);
	ledcAttachPin(1, GREEN);
	ledcSetup(2, freq, resolution);
	ledcAttachPin(3, BLUE);

	printCenter("TEST", 64 - 16);
	u8g2.sendBuffer();
	u8g2.clearBuffer();

	ledcWrite(RED, 10);
	ledcWrite(GREEN, 0);
	ledcWrite(BLUE, 0);
	delay(250);
	ledcWrite(RED, 0);
	ledcWrite(GREEN, 10);
	ledcWrite(BLUE, 0);
	delay(250);
	ledcWrite(RED, 0);
	ledcWrite(GREEN, 0);
	ledcWrite(BLUE, 10);
	delay(250);
	ledcWrite(RED, 0);
	ledcWrite(GREEN, 0);
	ledcWrite(BLUE, 0);

	Serial.println("Setting pins");
	for (int i = 4; i <= 7; i++)
	{
		pinMode(i, INPUT);
	}

	// TX, RX, EN
	dmx_set_pin(dmxPort, tx_pin, rx_pin, rts_pin);

	for (int i = 0; i < DMX_PACKET_SIZE; i++)
	{
		data[i] = 0;	// Initialize data buffer
		data0[i] = 0; // Initialize data0 buffer
	}
}

bool strobeState = 0;
uint8_t strobe = 0;
unsigned long lastStrobe = 0;

bool gotData = 0;

void loop()
{
	if (remainingFrames > 0)
	{
		uint16_t addr = dmx_get_start_address(dmxPort);
		String addrStr;
		if (addr < 10)
			addrStr += "0";
		if (addr < 100)
			addrStr += "0";
		addrStr += String(dmx_get_start_address(dmxPort));
		printCenter(String(addrStr).c_str(), 64 - 16);

		u8g2.sendBuffer();
		u8g2.clearBuffer();
		remainingFrames--;
	}

	for (int i = 0; i < 4; i++)
	{
		if (!digitalRead(buttonPins[i]) != buttonStatesBef[i])
		{
			buttonStatesBef[i] = !digitalRead(buttonPins[i]);
			if (buttonStatesBef[i])
			{

				remainingFrames++;
				switch (i)
				{
				case 0:
				{
					// MODE
					break;
				}
				case 1:
				{
					// UP
					dmx_set_start_address(dmxPort, dmx_get_start_address(dmxPort) + 1);
					break;
				}
				case 2:
				{
					// DOWN
					dmx_set_start_address(dmxPort, dmx_get_start_address(dmxPort) - 1);
					break;
				}
				case 3:
				{
					u8g2.sendBuffer();
					remainingFrames++;
					preferences.putUShort("addr", dmx_get_start_address(dmxPort));
					// ENTER
					break;
				}
				}
			}
		}
	}

	int size = dmx_receive(dmxPort, &packet, 4 * DMX_TIMEOUT_TICK);
	if (size > 0)
	{
		dmx_read(dmxPort, data, size);

		gotData = 1;

		// Optionally handle RDM requests
		if (packet.is_rdm)
		{
			rdm_send_response(dmxPort);
		}
		else
		{
			// Serial.printf("Received DMX packet: Start Code: %d, Size: %zu, RDM: %s, Start channel: %d\n",
			//							packet.sc, packet.size, packet.is_rdm ? "Yes" : "No", dmx_get_start_address(dmxPort));
			for (int i = 0; i < size; i++)
			{
				if (data0[i] != data[i])
				{
					data0[i] = data[i]; // Update the previous data buffer
					Serial.printf("Data[%d]: %d\n", i, data[i]);
				}
			}

			uint16_t startAddress = dmx_get_start_address(dmxPort);
			uint8_t dimmer = data[startAddress] / 2;
			uint8_t red = data[startAddress + 1];
			uint8_t green = data[startAddress + 2];
			uint8_t blue = data[startAddress + 3];
			strobe = data[startAddress + 4];
			if (strobe == 0)
				strobeState = 1;
			Serial.print("Red: ");
			Serial.print(red);
			Serial.print(" Green: ");
			Serial.print(green);
			Serial.print(" Blue: ");
			Serial.print(blue);
			Serial.print(" gotData:");
			Serial.println(gotData);

			uint8_t corr = data[startAddress + 5];
			if (corr == 0)
				corr = 1;
			// Multiply then divide with enough precision:
			uint32_t numerator = (uint32_t)dimmer * red * strobeState * 128;
			uint32_t raw = numerator / corr;
			uint32_t duty = raw / 256;
			if (duty > 255)
				duty = 255;
			ledcWrite(RED, duty);
			ledcWrite(GREEN, (dimmer * green * strobeState) / 256);
			ledcWrite(BLUE, (dimmer * blue * strobeState) / 256);
		}
		// Process data here...
	}
	else
	{
		ledcWrite(RED, 0);
		ledcWrite(GREEN, 0);
		ledcWrite(BLUE, 0);

		Serial.println("No data");

		gotData = 0;
	}

	if (strobe != 0 && millis() - lastStrobe > (1000 / map(strobe, 0, 255, 1, 30)))
	{
		lastStrobe = millis();
		strobeState = !strobeState;
	};
	// Do other work here...
}

#endif