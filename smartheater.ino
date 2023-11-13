
#include <ESP8266WiFi.h>
#include "FS.h"
#include <PubSubClient.h>
#include <DHT.h>  // library for getting data from DHT
#include <NTPClient.h>      // Network Time Protocol Client, used to validate certificates
#include <WiFiUdp.h>        // UDP to communicate with the NTP server



// Enter your WiFi ssid and password
char ssid[] = "[YOUR_SSID]"; //Provide your SSID
const char* password = "[password]"; // Provide Password
const char* mqtt_server = "[ENDPOINT]-ats.iot.[REGION].amazonaws.com"; // Relace with your MQTT END point
const int   mqtt_port = 8883;
const unsigned int MODULE_RELAY_ON = LOW;
const unsigned int MODULE_RELAY_OFF = HIGH;

#define BUFFER_LEN 256
#define DHTPIN 5        //pin where the DHT22 is connected 
#define RELAY_1 4
#define RELAY_2 3

long lastMsg = 0;
char msg[BUFFER_LEN];
int Value = 0;
byte mac[6];
char mac_Id[18];
int count = 1;

WiFiClientSecure espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

DHT dht(DHTPIN, DHT11);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

   timeClient.begin();
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }

  espClient.setX509Time(timeClient.getEpochTime());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
   
    String clientId = "ESP8266-kalorifer";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("ei_out", "hello world");
      // ... and resubscribe
      client.subscribe("ei_in");
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
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(2, OUTPUT);
  setup_wifi();
  delay(1000);
  //=============================================================
  if (SPIFFS.begin()){Serial.println(" mounting SPIFFS succesfully");
    } else{
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  //=======================================
  //Root CA File Reading.
  File rootca = SPIFFS.open("/AmazonRootCA1.der", "r");
  if (!rootca) {
    Serial.println("Failed to open ca file for reading");
    return;
  }
 
  //=============================================
  // Cert file reading
  File cert = SPIFFS.open("/cert.der", "r");
  
  if (!cert) {
    Serial.println("Failed to open cert file for reading");
    return;
  }
  
  //=================================================
  //Privatekey file reading
  File privatekey = SPIFFS.open("/private.der", "r");
  if (!privatekey) {
    Serial.println("Failed to open private key file for reading");
    return;
  }

  //=====================================================

  espClient.loadCACert(rootca);
  espClient.loadPrivateKey(privatekey);
  espClient.loadCertificate(cert);
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //====================================================================================================================
  WiFi.macAddress(mac);
  snprintf(mac_Id, sizeof(mac_Id), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(mac_Id);
  //=====================================================================================================================
  delay(2000);
}


void loop() {
  float h = dht.readHumidity();   // Reading Temperature form DHT sensor
  float t = dht.readTemperature();      // Reading Humidity form DHT sensor
  
 //===================TEMP_ACTUATORS-AUTOMODE=======
 
   switch (t) {
  case 20.0:
   CloseHeater1()
   OpenHeater2()
    break;
  case 21.0:
   CloseHeater2()
   OpenHeater1()
    break;
  case 22.0:
   CloseHeater1()
    break;
  default:
    // statements
    break;
}

 //=======================================
  if (isnan(h) || isnan(t))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  //print the values of DHT
  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("    Humidity = ");
  Serial.print(h);
  Serial.println(" % ");
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    //=============================================================================================
   // String macIdStr = mac_Id;
    String Temprature = String(t);
    String Humidity = String(h);
    snprintf (msg, BUFFER_LEN, "{\"client_Id\" : \"%s\", \"Temprature\" : %s, \"Humidity\" : \"%s\"}", clientId.c_str(), Temprature.c_str(), Humidity.c_str());
    Serial.print("Publish message: ");
    Serial.print(count);
    Serial.println(msg);
    client.publish("temp", msg);
    count = count + 1;
    //================================================================================================
  }
    delay(10000);            // wait for a 10 seconds until next reading and publishing
}

//=====METHODS=====

  void OpenHeater1(){
     digitalWrite(RELAY_1,MODULE_RELAY_ON); 
  }
  void CloseHeater1(){
     digitalWrite(RELAY_1,MODULE_RELAY_OFF); 
     }
  void OpenHeater2(){
     digitalWrite(RELAY_2,MODULE_RELAY_ON); 
  }
  void CloseHeater2(){
     digitalWrite(RELAY_2,MODULE_RELAY_OFF); 
     }
