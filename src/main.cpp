#include <Arduino.h>
#if OTA
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
const char *ssid = "BoldogUtca_IOT";
const char *password = "12345678";
bool update = 0;

#endif

#include <esp_dmx.h>

#define DEBUG 0

#define ISBETWEEN(lowEnd, test, highEnd) (lowEnd <= test && test <= highEnd)

/* First, lets define the hardware pins that we are using with our ESP32. We
need to define which pin is transmitting data and which pin is receiving data.
DMX circuits also often need to be told when we are transmitting and when we
are receiving data. We can do this by defining an enable pin. */

/*
2	  OK	OK
4	  OK	OK
13	OK	OK
14	OK
16	OK	OK
17	OK	OK
18	OK	OK
19	OK	OK
21	OK	OK
22	OK	OK
23	OK	OK
25	OK	OK
26	OK	OK
27	OK	OK
32	OK	OK
33	OK	OK
34	OK
35	OK
36	OK
39	OK
*/

int transmitPin = 17;
int receivePin = 16;
int enablePin = 18;

int smokePin = 19;
int fanPin = 21;

// define DMX address selector pins
// int horizontal[] = {32, 33};
// int vertical[] = {25, 26, 27, 13, 4};

int startAddress = 0;

/* Next, lets decide which DMX port to use. The ESP32 has either 2 or 3 ports.
	Port 0 is typically used to transmit serial data back to your Serial Monitor,
	so we shouldn't use that port. Lets use port 1! */
dmx_port_t dmxPort = 1;

/* Now we want somewhere to store our DMX data. Since a single packet of DMX
	data can be up to 513 bytes long, we want our array to be at least that long.
	This library knows that the max DMX packet size is 513, so we can fill in the
	array size with `DMX_PACKET_SIZE`. */
byte data[DMX_PACKET_SIZE];

/* The last two variables will allow us to know if DMX has been connected and
	also to update our packet and print to the Serial Monitor at regular
	intervals. */
bool dmxIsConnected = false;
#if DEBUG
unsigned long lastUpdate = millis();
#endif // DEBUG

void setup()
{
#if DEBUG
	Serial.begin(115200);
	Serial.println("Booting");
#endif
#if OTA
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.waitForConnectResult() != WL_CONNECTED)
	{
#if DEBUG
		Serial.println("Connection Failed! Rebooting...");
#endif
		delay(5000);
		ESP.restart();
	}

	// Hostname defaults to esp3232-[MAC]
	ArduinoOTA.setHostname("fustgep");

	// No authentication by default
	ArduinoOTA.setPassword("admin");
#endif

	pinMode(smokePin, OUTPUT);
	pinMode(fanPin, OUTPUT);

	digitalWrite(smokePin, LOW);
	digitalWrite(fanPin, LOW);

	/* Now we will install the DMX driver! We'll tell it which DMX port to use,
		what device configuration to use, and what DMX personalities it should have.
		If you aren't sure which configuration to use, you can use the macros
		`DMX_CONFIG_DEFAULT` to set the configuration to its default settings.
		This device is being setup as a DMX responder so it is likely that it should
		respond to DMX commands. It will need at least one DMX personality. Since
		this is an example, we will use a default personality which only uses 1 DMX
		slot in its footprint. */
	dmx_config_t config = DMX_CONFIG_DEFAULT;
	dmx_personality_t personalities[] = {
			{2, "Default Personality"},
			{3, "OTA Personality"}};
	int personality_count = 2;
	dmx_driver_install(dmxPort, &config, personalities, personality_count);

	/* Now set the DMX hardware pins to the pins that we want to use and setup
		will be complete! */
	dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
#if OTA
	ArduinoOTA
			.onStart([]()
							 {
								 String type;
								 if (ArduinoOTA.getCommand() == U_FLASH)
								 {
									 type = "sketch";
								 }
								 else
								 { // U_SPIFFS
									 type = "filesystem";
								 }

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
#if DEBUG
								 Serial.println("Start updating " + type);
#endif // DEBUG
							 })
			.onEnd([]()
						 {
#if DEBUG
							 Serial.println("\nEnd");
#endif // DEBUG
						 })
			.onProgress([](unsigned int progress, unsigned int total)
									{
#if DEBUG
										Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
#endif // DEBUG
									})
#if DEBUG

			.onError([](ota_error_t error)
							 {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      } })
#endif // DEBUG
			;

	ArduinoOTA.begin();

#if DEBUG
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
#endif // DEBUG
#endif // OTA
}

void loop()
{
#if OTA
	if (update)
		ArduinoOTA.handle();
#endif // OTA
	/* We need a place to store information about the DMX packets we receive. We
	will use a dmx_packet_t to store that packet information.  */
	dmx_packet_t packet;

	/* And now we wait! The DMX standard defines the amount of time until DMX
		officially times out. That amount of time is converted into ESP32 clock
		ticks using the constant `DMX_TIMEOUT_TICK`. If it takes longer than that
		amount of time to receive data, this if statement will evaluate to false. */
	if (dmx_receive_num(dmxPort, &packet, 10, 0))
	{
		/* If this code gets called, it means we've received DMX data! */

		/* Get the current time since boot in milliseconds so that we can find out
			how long it has been since we last updated data and printed to the Serial
			Monitor. */
		unsigned long now = millis();

		/* We should check to make sure that there weren't any DMX errors. */
		if (!packet.err)
		{
			/* If this is the first DMX data we've received, lets log it! */
			if (!dmxIsConnected)
			{
#if DEBUG
				Serial.println("DMX is connected!");
#endif // DEBUG
				dmxIsConnected = true;
			}

			/* Don't forget we need to actually read the DMX data into our buffer so
				that we can print it out. */
			dmx_read(dmxPort, data, packet.size);

#if DEBUG
			if (now - lastUpdate > 1000)
			{

				/* Print the received start code - it's usually 0. */
				Serial.printf("Start code is 0x%02X and slot 1 is 0x%02X\n", data[0],
											data[1]);
				lastUpdate = now;
			}
#endif
			if (data[0] == 0) // byte 0 is 0, we received DMX
			{

				digitalWrite(smokePin, ISBETWEEN(100, data[startAddress + 1], 200));
				digitalWrite(fanPin, ISBETWEEN(100, data[startAddress + 2], 200));
#if OTA
				update = data[startAddress + 3] > 127;
#endif
			}
		}
		else
		{
			/* Oops! A DMX error occurred! Don't worry, this can happen when you first
				connect or disconnect your DMX devices. If you are consistently getting
				DMX errors, then something may have gone wrong with your code or
				something is seriously wrong with your DMX transmitter. */
#if DEBUG
			Serial.println("A DMX error occurred.");

#endif // DEBUG
		}
	}
	else if (dmxIsConnected)
	{
		/* If DMX times out after having been connected, it likely means that the
			DMX cable was unplugged. When that happens in this example sketch, we'll
			uninstall the DMX driver. */
#if DEBUG
		Serial.println("DMX was disconnected.");
#endif // DEBUG
	}
	else
	{
#if DEBUG
		Serial.println("DMX is disconnected");
#endif // DEBUG

		digitalWrite(smokePin, LOW);
		digitalWrite(fanPin, LOW);
	}
}
