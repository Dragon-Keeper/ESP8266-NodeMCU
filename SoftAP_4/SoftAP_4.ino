
#include <ESP8266WiFi.h>

IPAddress local_IP(192,168,2,110);  
IPAddress gateway(192,168,2,1);  
IPAddress subnet(255,255,255,0); 
 
void setup() {  
  // put your setup code here, to run once:  
  Serial.begin(115200);
  WiFi.softAPConfig(local_IP, gateway, subnet);  
  WiFi.softAP("SoftAP001", "123456789");  
  Serial.print("Soft-AP IP address = ");  
  Serial.println(WiFi.softAPIP());  
}  
  
void loop() {  
  // put your main code here, to run repeatedly:  
  Serial.print("Soft-AP has biuld");
  delay(5000);
}
