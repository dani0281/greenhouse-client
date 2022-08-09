#include <Arduino.h>

#include <SPI.h>
#include <LwIP.h>
#include <STM32Ethernet.h>
#include <PubSubClient.h>

//IPAddress server(10, 20, 0, 9);
IPAddress server(192, 168, 1, 2);

void callback(char *topic, byte *payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

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
}

void loop()
{
  client.loop();
}