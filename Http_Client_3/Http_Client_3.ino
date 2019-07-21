
#include <Arduino.h>  
#include <ESP8266WiFi.h>  
#include <ESP8266WiFiMulti.h>  
#include <ESP8266HTTPClient.h>  
#include"hz.c"
ESP8266WiFiMulti WiFiMulti;  
  
#define HTTPIP "120.77.168.220" //bbs.gz0668.com
#define HTTPPORT 80  
#define WIFINAME hz[0]
#define WIFIPW "TF06680668" 
  
  
void setup() {  
  // put your setup code here, to run once:  
  Serial.begin(115200);  
  Serial.println();  
  Serial.println("Connecting");  
  WiFiMulti.addAP(WIFINAME,WIFIPW);  
  while(WiFiMulti.run()!=WL_CONNECTED)  
  {  
    delay(500);  
    Serial.print(".");  
  }  
  Serial.println("Connected!");  
}  
  
void loop() {    
  HTTPClient http;  
  Serial.println("Try link to http.");  
  http.begin(HTTPIP,HTTPPORT,"/");  
  int Code = http.GET();  
  if(Code)  
  {  
    Serial.printf("HTTP Code:%d\n",Code);  
    if(Code == 200)  
    {  
      String payload = http.getString(); 
      //由于获取的编码以ANSI格式保存，所以无法正确显示UTF-8格式的中文  
      Serial.println(payload);  
    }  
    else  
    {  
      Serial.println("Couldn't link to server");  
    }  
  }  
  delay(5000);  
}  
