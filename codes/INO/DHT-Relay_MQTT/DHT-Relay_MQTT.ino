//Using ESP8266
//using library DHT sensor Library by Adafruit Version 1.4.3
//This program for send temp, humidity to mqtt broker
//Receive message from mqtt broker to turn on device
//Reference GPIO  https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

//Setup for MQTT and WiFi============================
#include <ESP8266WiFi.h>
//Library for MQTT:
#include <PubSubClient.h>
//Library for Json format using version 5:
#include <ArduinoJson.h>

//Setup for DHT======================================
#include <DHT.h>
#define DHTPIN 2  //GPIO2 atau D4
// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT11     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

//declare topic for publish message
const char* topic_pub = "ESP_Pub";
//declare topic for subscribe message
const char* topic_sub = "ESP_Sub";

// Update these with values suitable for your network.
const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* mqtt_server = "103.227.130.104";

//for output
int lamp1 = 16; //lamp for mqtt connected D0
int lamp2 = 5; //lamp for start indicator D1
int lamp3 = 4; //lamp for stop indicator D2

WiFiClient espClient;
PubSubClient client(espClient);
//char msg[50];

void setup_wifi() {
  delay(100);
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
  //Receiving message as subscriber
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String json_received;
  Serial.print("JSON Received:");
  for (int i = 0; i < length; i++) {
    json_received += ((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println(json_received);
  //if receive ask status from node-red, send current status of lamps
  if (json_received == "Status")
  {
    check_stat();
  }
  else
  {
    //Parse json
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json_received);

    //get json parsed value
    //sample of json: {"device":"Lamp1","trigger":"on"}
    Serial.print("Command:");
    String device = root["device"];
    String trigger = root["trigger"];
    Serial.println("Turn " + trigger + " " + device);
    Serial.println("-----------------------");
    //Trigger device
    //Lamp1***************************
    if (device == "Lamp1")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp1, LOW);
      }
      else
      {
        digitalWrite(lamp1, HIGH);
      }
    }
    //Lamp2***************************
    if (device == "Lamp2")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp2, LOW);
      }
      else
      {
        digitalWrite(lamp2, HIGH);
      }
    }
    //Lamp3***************************
    if (device == "Lamp3")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp3, LOW);
      }
      else
      {
        digitalWrite(lamp3, HIGH);
      }
    }
    //All***************************
    if (device == "All")
    {
      if (trigger == "on")
      {
        digitalWrite(lamp1, LOW);
        digitalWrite(lamp2, LOW);
        digitalWrite(lamp3, LOW);
      }
      else
      {
        digitalWrite(lamp1, HIGH);
        digitalWrite(lamp2, HIGH);
        digitalWrite(lamp3, HIGH);
      }
    }
    check_stat();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      //once connected to MQTT broker, subscribe command if any
      client.subscribe(topic_sub);
      check_stat();
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
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //subscribe topic
  client.subscribe(topic_sub);
  //setup pin output
  pinMode(lamp1, OUTPUT);
  pinMode(lamp2, OUTPUT);
  pinMode(lamp3, OUTPUT);
  //Reset lamp, turn off all Relay
  digitalWrite(lamp1, HIGH);
  digitalWrite(lamp2, HIGH);
  digitalWrite(lamp3, HIGH);
}

void check_stat()
{
  //check output status--------------------------------------------
  //This function will update lamp status to mqtt
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  bool stat_lamp1 = digitalRead(lamp1);
  bool stat_lamp2 = digitalRead(lamp2);
  bool stat_lamp3 = digitalRead(lamp3);
  //lamp1==========================
  if (stat_lamp1 == false)
  {
    JSONencoder["lamp1"] = true;
  }
  else
  {
    JSONencoder["lamp1"] = false;
  }
  //lamp2==========================
  if (stat_lamp2 == false)
  {
    JSONencoder["lamp2"] = true;
  }
  else
  {
    JSONencoder["lamp2"] = false;
  }
  //lamp3==========================
  if (stat_lamp3 == false)
  {
    JSONencoder["lamp3"] = true;
  }
  else
  {
    JSONencoder["lamp3"] = false;
  }


  JSONencoder["device"] = "ESP8266";
  JSONencoder["temperature"] = t;
  JSONencoder["humidity"] = h;

  char JSONmessageBuffer[100];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Sending message to MQTT topic..");
  Serial.println(JSONmessageBuffer);
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  if (client.publish(topic_pub, JSONmessageBuffer) == true) {
    Serial.println("Success sending message");
  } else {
    Serial.println("Error sending message");
  }
  Serial.println("-------------");
}

void loop() {
  delay(1000);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //read DHT sensor, temp and humidity-------------------------------
  t = dht.readTemperature();
  h = dht.readHumidity();
  if ((isnan(t)) || (isnan(h)))
  {
    Serial.println("Failed to read from DHT sensor!");
  }
}
