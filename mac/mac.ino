#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
//FirebaseESP8266.h must be included before ESP8266WiFi.h
#include "FirebaseESP8266.h"
// #include <credentials.h>
#include <set>
#include <string>
#include "./functions.h"
#include "./mqtt.h"

// Create Wi-Fi and Firebase credentials
// Replace the values with your own Firebase credentials 

#define FIREBASE_HOST "wifiscanner-3d498.firebaseio.com" //Do not include https:// in FIREBASE_HOST
#define FIREBASE_AUTH "ajzjoWXwSshENIEjHOTdLgCK3q9CVKLe35mFZO5T"

#define disable 0
#define enable  1
#define SENDTIME 30000
#define MAXDEVICES 60
#define JBUFFER 15+ (MAXDEVICES * 40)
#define PURGETIME 600000
#define MINRSSI -70

// uint8_t channel = 1;
unsigned int channel = 1;
int clients_known_count_old, aps_known_count_old;
unsigned long sendEntry, deleteEntry;
char jsonString[JBUFFER];


String device[MAXDEVICES];
int nbrDevices = 0;
int count = 0;
int usedChannels[15];

#ifndef CREDENTIALS
//#define mySSID "Jason"
//#define myPASSWORD "karomatullo123"
//#define mySSID "DataMicron"
//#define myPASSWORD "instabi@1234"

//#define mySSID "AndroidAP_9658"
//#define myPASSWORD "sardor1992"

#define mySSID "B-20-3-TIME2.4Ghz"
#define myPASSWORD "01139040034"

#endif

StaticJsonBuffer<JBUFFER>  jsonBuffer;
//Define Firebase Data object
FirebaseData firebaseData;
// Define firebase RealTime database path
String path = "/HumanCounter";
String jsonData = "";

FirebaseJson macAddress;
FirebaseJsonArray macJsonArray;
FirebaseJson json;
FirebaseJsonArray jsonArr2;


void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.printf("\n\nSDK version:%s\n\r", system_get_sdk_version());

    wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
    wifi_set_channel(channel);
    wifi_promiscuous_enable(disable);
    wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
    wifi_promiscuous_enable(enable);
}

void loop() {
  channel = 1;
  boolean sendFirebase = false;

  wifi_set_channel(channel);
  while (true) {
    nothing_new++;                          // Array is not finite, check bounds and adjust if required
    if (nothing_new > 200) {                // monitor channel for 200 ms
      nothing_new = 0;
      channel++;
      if (channel == 15) break;             // Only scan channels 1 to 14
      wifi_set_channel(channel);
    }
    delay(1);  // critical processing timeslice for NONOS SDK! No delay(0) yield()

    if (clients_known_count > clients_known_count_old) {
      clients_known_count_old = clients_known_count;
      sendFirebase = true;
    }
    if (aps_known_count > aps_known_count_old) {
      aps_known_count_old = aps_known_count;
      sendFirebase = true;
    }
    if (millis() - sendEntry > SENDTIME) {
      sendEntry = millis();
      sendFirebase = true;
    }
  }

  purgeDevice();
  if (sendFirebase) {
    showDevices();
    sendDevices();
  }

}

void connectToWiFi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(mySSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(mySSID, myPASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("");
  Serial.println("Initializing Firebase");
  //initFirebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Serial.println("");

}

void purgeDevice() {

  for (int u = 0; u < clients_known_count; u++) {
    if ((millis() - clients_known[u].lastDiscoveredTime) > PURGETIME) {
      Serial.print("purge Client" );
      Serial.println(u);
      for (int i = u; i < clients_known_count; i++) memcpy(&clients_known[i], &clients_known[i + 1], sizeof(clients_known[i]));
      clients_known_count--;
      break;
    }
  }

  for (int u = 0; u < aps_known_count; u++) {
    if ((millis() - aps_known[u].lastDiscoveredTime) > PURGETIME) {
      Serial.print("purge Bacon" );
      Serial.println(u);
      for (int i = u; i < aps_known_count; i++) memcpy(&aps_known[i], &aps_known[i + 1], sizeof(aps_known[i]));
      aps_known_count--;
      break;
    }
  }

}


void showDevices() {
  Serial.println("");
  Serial.println("");
  Serial.println("-------------------Device DB-------------------");
  Serial.printf("%4d Devices + Clients.\n",aps_known_count + clients_known_count); // show count

  // show Beacons
  for (int u = 0; u < aps_known_count; u++) {
    Serial.printf( "%4d ",u); // Show beacon number
    Serial.print("B ");
    Serial.print(formatMac1(aps_known[u].bssid));
    Serial.print(" RSSI ");
    Serial.print(aps_known[u].rssi);
    Serial.print(" channel ");
    Serial.println(aps_known[u].channel);
  }

  // show Clients
  for (int u = 0; u < clients_known_count; u++) {
    Serial.printf("%4d ",u); // Show client number
    Serial.print("C ");
    Serial.print(formatMac1(clients_known[u].station));
    Serial.print(" RSSI ");
    Serial.print(clients_known[u].rssi);
    Serial.print(" channel ");
    Serial.println(clients_known[u].channel);
  }
}

void sendDevices() {

  String deviceMac;

  // Disable WiFi Promiscuous mode to connect to internet
  wifi_promiscuous_enable(disable);
  connectToWiFi();

  // Purge json string
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& mac = root.createNestedArray("MAC");
  // JsonArray& rssi = root.createNestedArray("RSSI");

  // add Beacons
  for (int u = 0; u < aps_known_count; u++) {
    deviceMac = formatMac1(aps_known[u].bssid);
    if (aps_known[u].rssi > MINRSSI) {
      mac.add(deviceMac);
      //    rssi.add(aps_known[u].rssi);
    }
  }

  // Add Clients
  for (int u = 0; u < clients_known_count; u++) {
    deviceMac = formatMac1(clients_known[u].station);
    if (clients_known[u].rssi > MINRSSI) {
      mac.add(deviceMac);
      
      macAddress.addString("mac", deviceMac);
      json.addString("mac", deviceMac);      
      Serial.println("------------------------------------");
      Serial.println("Set JSON to Firebase...");

      if (Firebase.pushJSON(firebaseData, path, json)){
          Serial.println("PASSED");
          Serial.println("PATH: " + firebaseData.dataPath());
          Serial.println("TYPE: " + firebaseData.dataType());
          Serial.print("VALUE: ");
          Serial.println(firebaseData.jsonData());      
          Serial.println("------------------------------------");
          Serial.println();
      }else{
          Serial.println("FAILED");
          Serial.println("REASON: " + firebaseData.errorReason());
          Serial.println("------------------------------------");
          Serial.println();
      }
    }
  }
  
  Serial.println();
  Serial.printf("number of devices: %02d\n", mac.size());
  root.prettyPrintTo(Serial);
  root.printTo(jsonString);
  delay(100);
  wifi_promiscuous_enable(enable);
  sendEntry = millis();
}
