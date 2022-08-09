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

int light = 0;

int a;
float temperature;
int B = 3975; // B value of the thermistor
float resistance;

void temperatureCallback()
{
  a = analogRead(A1);
  resistance = (float)(1023 - a) * 10000 / a;
  temperature = 1 / (log(resistance / 10000) / B + 1 / 298.15) - 273.15;

  char result[5];

  dtostrf(temperature, 5, 2, result);

  client.publish(temperature_topic, result);

  delay(1000);
}

void lightCallback()
{
  light = analogRead(A2);
  int lightOn = light >= 400 ? 1 : 0;

  char result[1];

  sprintf(result, "%d", lightOn);

  client.publish(light_topic, result);

  delay(1000);
}

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

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // this check is only needed on the Leonardo:
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  initializeEthernet();

  Serial.println("Hello World!!!!\r\n");

  if (client.connect("arduinoClient"))
  {
    client.publish("outTopic", "hello world");
    client.subscribe("inTopic");
  }

  temperatureThread.onRun(temperatureCallback);
  lightThread.onRun(lightCallback);
}

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
}