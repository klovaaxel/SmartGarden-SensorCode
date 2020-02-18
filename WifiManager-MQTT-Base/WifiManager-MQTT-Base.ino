#include <ESP8266WiFi.h>    //https://github.com/esp8266/Arduino
//for storing data
#include "EEPROM.h"

//needed for library
  #include <DNSServer.h>
  #include <ESP8266WebServer.h>
  #include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

//MQTT 
  #include <PubSubClient.h>
  std::string sensNum = "/klovakarlsson@gmail.com/01001"; //----------------------------------------------Change This

//mosit
  int moistPin = A0;
  String moist_str;
  char moist[50];

//temp
  #include <dht.h>
  dht DHT;
  #define DHT11_PIN 2
  
  String temp_str;
  char temp[50];

const char* mqtt_server = "mqtt.dioty.co";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"klovakarlsson@gmail.com", "ee07432c")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe("/klovakarlsson@gmail.com/01001");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {

    //moist
    //pinMode(inPin, INPUT);
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("serial started");

    EEPROM.begin(512);


    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("SmartSensor1.0AP");
    
    //if you get here you have connected to the WiFi
    Serial.println("connected");


    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  //moist
  int sensorValue = analogRead(moistPin);
  sensorValue = 100 - (sensorValue / 10);
  Serial.println(sensorValue);
  moist_str = String(sensorValue) + "%"; //converting ftemp (the float variable above) to a string
  Serial.println(moist_str);
  moist_str.toCharArray(moist, moist_str.length() + 1);
  
  //temp
  int chk = DHT.read11(DHT11_PIN);
  temp_str = String(DHT.temperature) + "°C"; //converting ftemp (the float variable above) to a string
  temp_str.toCharArray(temp, temp_str.length() + 1);
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    Serial.print("Publish message: ");
    Serial.print(temp);
    Serial.print(", ");
    Serial.print(moist);
    Serial.println();
    
    std::string moistChannel = sensNum + "/moist";
    std::string tempChannel = sensNum + "/temp";

    Serial.println(moistChannel.c_str());


    client.publish((tempChannel.c_str()), temp);
    client.publish((moistChannel.c_str()), moist);
    delay(5000);
  }

}
