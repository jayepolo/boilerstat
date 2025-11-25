/*

Jay Polo

Boiler Monitor will monitor the operation of oil fired boiler controls and transmit status periodically.

It is designed to communicate with another ESP8266, which can act as a bridge

to another device or cloud database

*/

#include <esp_now.h>

#include <WiFi.h>

//#include <esp_wifi.h> // only for esp_wifi_set_channel()

// REPLACE WITH YOUR RECEIVER MAC Address

//uint8_t broadcastAddress[] = {0x18, 0xFE, 0x34, 0xD7, 0x7D, 0x90};

//uint8_t broadcastAddress[] = {0x5C, 0xCF, 0x7F, 0x0B, 0xF3, 0x17}; This is broken ESP8266

uint8_t broadcastAddress[] = {0x18, 0xFE, 0x34, 0xD3, 0x9A, 0xBC}; // This is the new ESP8266

&nbsp;

// Structure example to send data

// Must match the receiver structure

typedef struct struct_message {

// char MAC[1];

//String MAC;

unsigned long sample;

bool z1;

bool z2;

bool z3;

bool z4;

bool z5;

bool z6;

bool boiler;

} struct_message;

// Create a struct_message called myData

struct_message myData;

String macAddr = WiFi.macAddress();

unsigned long lastTime = 0;

unsigned long timerDelay = 10000; // send readings timer

int LED = 5; //LED_BUILTIN;

int inBurner = 36;

int inXtra = 39;

int inZone1 = 32; // GPIO4 is pin D1 - Second down from top right

int inZone2 = 33; // GPIO5 is pin D2 - Third down from top right

int inZone3 = 34; // GPIO14 is pin D5 - Middle right side

int inZone4 = 35; // GPIO12 is pin D6 -

int inZone5 = 25; // GPIO13 is pin D7 -

int inZone6 = 26;

// int inZone1 = 4; // GPIO4 is pin D1 - Second down from top right

// int inZone2 = 4; // GPIO5 is pin D2 - Third down from top right

// int inZone3 = 14; // GPIO14 is pin D5 - Middle right side

// int inZone4 = 12; // GPIO12 is pin D6 -

// int inZone5 = 13; // GPIO13 is pin D7 -

// int inZone6 = 6;

// int inBurner = 7;

// bool inXtra = 0;

bool blinkFlag = false; // Are we blinking now?

unsigned long blinkStartTime = 0; // Time when we started the blink

unsigned long blinkDelay = 50; // Blink duration is 200 mSec

esp_now_peer_info_t peerInfo;

// callback when data is sent

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

}

// void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {

// Serial.println(sendStatus == 0 ? "Delivery Success" : "Delivery Fail");

// }

void setup() {

pinMode(LED, OUTPUT); // Initialize the LED_BUILTIN pin as an output

pinMode(inZone1, INPUT); // Initialize the Zone1 pin as an input

pinMode(inZone2, INPUT); // Initialize the Zone2 pin as an input

pinMode(inZone3, INPUT); // Initialize the Zone3 pin as an input

pinMode(inZone4, INPUT); // Initialize the Zone4 pin as an input

pinMode(inZone5, INPUT); // Initialize the Zone5 pin as an input

pinMode(inZone6, INPUT); // Initialize the Zone6 pin as an input

pinMode(inBurner, INPUT); // Initialize the Burner1 pin as an input

pinMode(inXtra, INPUT); // Initialize the Xtra pin as an input

// Init Serial Monitor

Serial.begin(115200);

// Set device as a Wi-Fi Station

WiFi.mode(WIFI_STA);

WiFi.disconnect();

// Init ESP-NOW

if (esp_now_init() != 0) {

Serial.println("Error initializing ESP-NOW");

return;

}

// esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

// Once ESPNow is successfully Init, we will register for Send CB to

// get the status of Trasnmitted packet

esp_now_register_send_cb(OnDataSent);

// // Register peer

// esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

memcpy(peerInfo.peer_addr, broadcastAddress, 6);

peerInfo.channel = 0;

peerInfo.encrypt = false;

// Add peer

if (esp_now_add_peer(&peerInfo) != ESP_OK){

Serial.println("Failed to add peer");

return;

} else {

Serial.println("Successfully added peer");

}

// myData.sample = 0;

}

void loop() {

// Sample data periodically

if ((millis() - lastTime) > timerDelay) {

lastTime = millis();

//blinkFlag = true;

getData();

sendData();

}

// Turn off blink if it was on long enough

if (blinkFlag == true) {

if ((millis() - blinkStartTime) > blinkDelay){

digitalWrite(LED, HIGH); // Turn the LED off by making the voltage HIGH

blinkFlag = false;

}

}

}

void getData(){

//strcpy(myData.a, "18:FE:34:D7:7D:90");

[//myData.MAC](# "//myData.MAC") = macAddr;

myData.sample = myData.sample + 1;

myData.z1 = !digitalRead(inZone1);

myData.z2 = !digitalRead(inZone2);

myData.z3 = !digitalRead(inZone3);

myData.z4 = !digitalRead(inZone4);

myData.z5 = !digitalRead(inZone5);

myData.z6 = !digitalRead(inZone6);

myData.boiler = !digitalRead(inBurner);

// Serial.println(!digitalRead(inBurner));

// myData.boiler = !digitalRead(inXtra); // Not yet configured on receiver

}

void sendData() {

blinkStartTime = millis();

blinkFlag = true;

digitalWrite(LED, LOW); // Turn the LED on (Note that LOW is the voltage level

uint8_t bs[sizeof(myData)];

memcpy(bs, &myData, sizeof(myData));

esp_now_send(NULL, bs, sizeof(myData));

//esp_now_send(0, (uint8_t *) &myData, sizeof(myData));

[//Serial.print](# "//Serial.print")(myData.MAC + " ");

Serial.print(myData.sample);

Serial.print(" ");

Serial.print(myData.z1);

Serial.print(myData.z2);

Serial.print(myData.z3);

Serial.print(myData.z4);

Serial.print(myData.z5);

Serial.print(myData.z6);

Serial.print(" ");

Serial.print(myData.boiler);

Serial.print(" ");

// esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

// if (result == ESP_OK) {

// [//Serial.println](# "//Serial.println")("Message sent to 18:FE:34:D7:7D:90");

// Serial.print("Message sent to 5C:CF:7F:0B:F3:17 ");

// Serial.print(digitalRead(inZone2));

// Serial.println(myData.z1);

// }

// else {

// Serial.println("Error sending");

// }

}
