#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

#define I2C_DEV_ADDR 0x55

const char* ssid = "MTN_4G_8C08D7";
const char* password = "Bleoobi2k21";
//link to google sheet for AWG 
//const char* scriptURL = "https://script.google.com/macros/s/AKfycbxAXk7f3HIQeVa1TidOe0rRp4knKim8w8EzhdoyECj46QrJi03FKi9bh-hI5hM457t8KQ/exec";
//link to google sheet Sjoerd used for testing
//const char* scriptURL = "https://script.google.com/macros/s/AKfycbxg9RNkQSNCmo-WltfSg_L_30SMwNLjriTABvHcQSdlwPJkw_78psGgDbofR6WGRQxUOQ/exec";
//link to google sheet for AWG on Rolfs account 
const char* scriptURL = "https://script.google.com/macros/s/AKfycby2lzKva8QuTRt0S-pr5eRDFuePeCX-bcAf3MaqO6E9B6QdFA8o2AyAQYed5HQkAyrWsQ/exec";

// Global buffer to accumulate incoming I2C chunks
String inputString = ""; 
volatile bool messageReady = false; 

//variables for timing "cool" message"
const long messageTime = 10000;
long prevTime = 0;

//pin for LED to indicate wifi status
const int wifiLED = 2; //build in

void parseAndSendData(String data);
void onReceive(int len);

void setup() {
  Serial.begin(115200);
  inputString.reserve(200); // Pre-allocate memory space

  //declare LED pinmode
  pinMode(wifiLED, OUTPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(wifiLED, HIGH);
    delay(1000);
    Serial.println("Connecting...");
  }
  digitalWrite(wifiLED, LOW);
  Serial.println("Connected to WiFi");

  Wire.onReceive(onReceive);
  Wire.begin((uint8_t)I2C_DEV_ADDR);
  Serial.printf("I2C Slave ready at address: 0x%02X\n", I2C_DEV_ADDR);
}

void loop() {
  // Check if the I2C interrupt flagged a completed message
  if (messageReady) {
    String messageToProcess = inputString;
    inputString = ""; // Reset buffer immediately
    messageReady = false; // Reset flag
    
    parseAndSendData(messageToProcess);
  }
  if ((millis() - prevTime) > messageTime){
    prevTime = millis();
    if (WiFi.status() == WL_CONNECTED){
      Serial.println("cool");
    } else {
      Serial.println("no connection to wifi, reconnecting");
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(wifiLED, HIGH);
        delay(1000);
        Serial.println("Connecting...");
      }
      digitalWrite(wifiLED, LOW);
      Serial.println("Connected to WiFi");
    }
  }
}

// I2C Callback context runs on an interrupt thread
void onReceive(int len) {
  Serial.println("receive");
  while (Wire.available()) {
    char inChar = Wire.read();
    
    if (inChar == '\n') {
      // End signal found! Signal the main loop to process it
      messageReady = true; 
    } else if (inChar != '\r') {
      inputString += inChar;
    }
  }
}

void parseAndSendData(String data) {
  int counter = 0;
  String time = "", hum1 = "", pres1 = "", temp1 = "", hum2 = "", pres2 = "", temp2 = "";
  String pres3 = "", temp3 = "", hum3 = "", amp = "", mass = "", v1 = "", v2 = "";
  
  //Serial.print("Processing accumulated line: ");
  //Serial.println(data);

  for (int i = 0; i < data.length(); i++){
    if (data.charAt(i) == ',') {
      counter += 1;
    } else {
      switch(counter){
        case 0:  time  += data[i]; break;
        case 1:  pres1 += data[i]; break;
        case 2:  temp1 += data[i]; break;
        case 3:  hum1  += data[i]; break;
        case 4:  pres2 += data[i]; break;
        case 5:  temp2 += data[i]; break;
        case 6:  hum2  += data[i]; break;
        case 7:  pres3 += data[i]; break;
        case 8:  temp3 += data[i]; break;
        case 9:  hum3  += data[i]; break;
        case 10: v1    += data[i]; break;
        case 11: v2    += data[i]; break;
        case 12: mass  += data[i]; break;
        case 13: amp   += data[i]; break;
        default: break;
      }
    }    
  }

  if (pres1 == "nan") pres1="-999";
  if (temp1 == "nan") temp1="-999";
  if (hum1  == "nan") hum1 ="-999";
  if (pres2 == "nan") pres2="-999";
  if (temp2 == "nan") temp2="-999";
  if (hum2  == "nan") hum2 ="-999";
  if (pres3 == "nan") pres3="-999";
  if (temp3 == "nan") temp3="-999";
  if (hum3  == "nan") hum3 ="-999";
  if (v1    == "nan") v1   ="-999";
  if (v2    == "nan") v2   ="-999";
  if (mass  == "nan") mass ="-999";
  if (amp   == "nan") amp  ="-999";

  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;  
    http.begin(scriptURL);
    http.addHeader("Content-Type", "application/json");
 
    String jsonData = "{\"time\": \"" + time + "\", \"temp1\": " + temp1 + ", \"hum1\": " + hum1 + ", \"pres1\": " + pres1 + ", \"pres2\": " + pres2 + ", \"temp2\": " + temp2 + ", \"hum2\": " + hum2 + ", \"pres3\": " + pres3 + ", \"temp3\": " + temp3 + ", \"hum3\": " + hum3 + ", \"v1\": " + v1 + ", \"v2\": \"" + v2 + "\"" + ", \"mass\": " + mass + ", \"amp\": " + amp + "}";
    Serial.println(jsonData);
    int httpResponseCode = http.POST(jsonData);
    Serial.println("Response code: " + String(httpResponseCode));
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}