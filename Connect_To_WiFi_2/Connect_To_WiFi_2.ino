
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include"hz.c"
 
#define WIFINAME hz[0]
#define WIFIPW   "TF06680668"
 
void setup() {
  // put your setup code here, to run once:
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  Serial.println("");
  WiFi.begin(WIFINAME, WIFIPW);
  Serial.print("Connecting..");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected,IP Address:");
  Serial.println(WiFi.localIP());
}
 
void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("I has connected to the WiFi!");
  delay(5000);
}
