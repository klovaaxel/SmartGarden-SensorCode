#include <ESP8266WiFi.h>    //https://github.com/esp8266/Arduino

#include "EEPROM.h" //for storing data in non volitile memory

//WIFI - needed for wifi server and connection
  #include <DNSServer.h>
  #include <ESP8266WebServer.h>
  #include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager

//MQTT - (communication)
  #include <PubSubClient.h>
  std::string sensNum = "/klovakarlsson@gmail.com/01001"; // Sens MQTT channel and sens number //------------Change This 
  const char* sensNumChar = sensNum.c_str(); //makes sensNum a const char
  
  const char* email = "klovakarlsson@gmail.com"; //mqtt email
  const char* password = "ee07432c"; //mqtt password (recived via mail)

//mosit
  int moistPin = A0;  // Pin that moist senor is connected to (A0 = ANALOG 0)
  String moist_str; // String that stores MositSens value
  char moist[50]; // Same as Moist_string but in format compatible with MQTT

//temp
  #include <dht.h>  // Tempsensor libary
  dht DHT;  //Defines DHT
  #define DHT11_PIN 2 // Pin that Temp Sensor is connected to
   
  String temp_str; // String that stores Temp value
  char temp[50]; // Same as temp_str but in format compatible with MQTT

const char* mqtt_server = "mqtt.dioty.co";  // the MQTT Broker address

WiFiClient espClient; // Defines Wifi to be ESP 
PubSubClient client(espClient); //Defines pubsubclient to use Wifi from espClient

long lastMsg = 0; // MQTT
char msg[50]; // MQTT
int value = 0;  // MQTT
void callback(char* topic, byte* payload, unsigned int length) { //function called when recieves message from subbed mqtt connection --- NOT USED 
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {  //Reconnect to MQTT broker
  // Loop until we're reconnected
  while (!client.connected()) { //while not conneted
    Serial.print("Attempting MQTT connection..."); //prints to serial 
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), email, password)) { //Attempts to connect to MQTT broker with email and password 

      Serial.println("connected");  // Once connected, publish an announcement...
      client.subscribe(sensNumChar);  // ... and resubscribe to sens channel and num

    } else { // if connection fails 
     
      Serial.print("failed, rc=");  
      Serial.print(client.state()); //prints connection error to serial
      Serial.println(" try again in 5 seconds");
    
      delay(5000);  // Wait 5 seconds before retrying
    }
  }
}

void setup() {  //code runs once at setup

    Serial.begin(115200); //Starts serial at 115200
    Serial.println("serial started"); //prints to serial

    EEPROM.begin(512);  //starts EEPROM at 512 (makes non volitile memmory available)


    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "SmartSensor1.0AP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("SmartSensor1.0AP");  
    
    //if you get here you have connected to the WiFi
    Serial.println("connected");

    //sets mqtt server to correct websocket and callback
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() { // put your main code here, to run repeatedly:
  
  //moist
  int sensorValue = analogRead(moistPin); //reads moist sensor
  sensorValue = 100 - (sensorValue / 10); //converts senorValue to a precentage
  //Serial.println(sensorValue);  //debug the moist value 
  moist_str = String(sensorValue) + "%"; //converting ftemp (the float variable above) to a string
  //Serial.println(moist_str);  //debug the moist value 
  moist_str.toCharArray(moist, moist_str.length() + 1); //converts string to char to be compatible with mqtt library
  
  //temp
  int chk = DHT.read11(DHT11_PIN);  //reads temp sensor
  temp_str = String(DHT.temperature) + "°C"; //converting ftemp (the float variable above) to a string
  temp_str.toCharArray(temp, temp_str.length() + 1);  //converts string to char to be compatible with mqtt library
  
  if (!client.connected()) {  //if client not connected call reconnect function
    reconnect();
  }
  
  client.loop();
  long now = millis();
  
  if (now - lastMsg > 2000) { //to not spam the mqtt (is bad)
    lastMsg = now;
    ++value;
    Serial.print("Publish message: ");
    Serial.print(temp);
    Serial.print(", ");
    Serial.print(moist);
    Serial.println();
    
    std::string moistChannel = sensNum + "/moist"; //creates a channel for moist value
    std::string tempChannel = sensNum + "/temp";  //creates a channel for temp value

    //Serial.println(moistChannel.c_str()); //debug for moist channel


    client.publish((tempChannel.c_str()), temp);  // publishes temp to temp channel on mqtt
    client.publish((moistChannel.c_str()), moist);  // publishes moist to moist channel on mqtt
    
    delay(5000); //waits for 5seconds (should idealy sleep for a set amount of time to save battery)
  }

}
