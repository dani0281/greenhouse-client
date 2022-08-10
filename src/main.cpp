/**
 * STM32 board application, using the Arduino framework to
 * setup a client, which can measure temperatures and light,
 * and publish them to a MQTT server.
 *
 * Special thanks to the PubSubClient for examples given,
 * explaining how to setup a MQTT client.
 *
 * @author Daniele A. Buttigli
 */

#include <Arduino.h>
#include <SPI.h>
#include <LwIP.h>
#include <STM32Ethernet.h>
#include <PubSubClient.h>
#include <Thread.h>

IPAddress server(192, 168, 1, 2);

void callback(char *topic, byte *payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

char *temperature_topic = "temperature/dk25-2";
char *light_topic = "light/dk25-2";

// Thread
Thread temperatureThread = Thread();
Thread lightThread = Thread();

int lightAnalogValue, temperatureAnalogValue;

float temperature, resistance;
int B = 3975; // B value of the thermistor

int publishCode = 0;

void wait(int s)
{
  delay(s * 1000);
}

/**
 * Ensure that the MQTT client is connected to the MQTT server.
 */
void ensureConnected()
{
  if (!client.connected())
  {
    while (!client.connect("dk25-2"))
    {
      wait(1);
    }

    Serial.println("");
    Serial.println("(Re)connected");
  }
}

/**
 * Publish the temperature to the MQTT server.
 */
void temperatureCallback()
{
  ensureConnected();

  temperatureAnalogValue = analogRead(A1);
  resistance = (float)(1023 - temperatureAnalogValue) * 10000 / temperatureAnalogValue;
  temperature = 1 / (log(resistance / 10000) / B + 1 / 298.15) - 273.15;

  char result[5];

  dtostrf(temperature, 5, 2, result);

  publishCode = client.publish(temperature_topic, result);
  Serial.println("Temperature: " + String(result));

  if (publishCode == 0)
  {
    Serial.println("Publish failed");
  }
}

/**
 * Publish the "light is on" value to the MQTT server.
 */
void lightCallback()
{
  ensureConnected();

  lightAnalogValue = analogRead(A2);
  int lightOn = lightAnalogValue >= 400 ? 1 : 0;

  char result[1];

  sprintf(result, "%d", lightOn);

  publishCode = client.publish(light_topic, result);
  Serial.println("Light on: " + String(result));

  if (publishCode == 0)
  {
    Serial.println("Publish failed");
  }
}

/**
 * Callback function for the MQTT client.
 * Copied from the PubSubClient example.
 */
void callback(char *topic, byte *payload, unsigned int length)
{
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Allocate the correct amount of memory for the payload copy
  byte *p = (byte *)malloc(length);

  // Copy the payload to the new buffer
  memcpy(p, payload, length);
  client.publish("outTopic", p, length);

  // Free the memory
  free(p);
}

/**
 * Print the current IPv4 address to the serial port.
 */
void printIPAddress()
{
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++)
  {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  Serial.println();
}

/**
 * Initialize the Ethernet connection.
 */
void initializeEthernet()
{
  // start the Ethernet connection:
  if (Ethernet.begin() == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }

  // print your local IP address:
  printIPAddress();
}

/**
 * Setup method for the application
 */
void setup()
{
  Serial.begin(9600);

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  initializeEthernet();

  temperatureThread.onRun(temperatureCallback);
  lightThread.onRun(lightCallback);
}

/**
 * Loop method for the application
 */
void loop()
{
  client.loop();

  if (temperatureThread.shouldRun())
  {
    temperatureThread.run();
  }

  if (lightThread.shouldRun())
  {
    lightThread.run();
  }

  wait(30);
}