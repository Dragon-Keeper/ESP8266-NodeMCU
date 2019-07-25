
#include <ESP8266WiFi.h>  
#include <ESP8266HTTPClient.h> 
#include"hz.c"
 
#define UKey ""           // Yeelink提供的U-ApiKey  
String ssid = hz[0];         // 需要连接的wifi热点名称  
String password = "TF06680668";     // 需要连接的wifi热点密码  
 
/* 上传数据到服务器。 
 * device为设备号，sensor为传感器号，data为上传数据点的值 
 * 这里默认上传到最新的数据点上，需要上传到特定点上就得在post内容中单独加入时间戳  
 */  
 
void uploadYeelinkData(String device, String sensor, String data) 
{  
  HTTPClient http;  
 
  const String apiAddress = "/v1.1/device/" + device + "/sensor/" + sensor + "/datapoints";  
  http.begin("api.yeelink.net", 80, apiAddress);  
  http.addHeader("U-ApiKey",UKey, true);  
  int httpCode =  http.POST("{\"value\":" + data + "}");  
  Serial.print("code:");  
  Serial.println(httpCode);  
  if(httpCode == 200) // 访问成功，取得返回参数 
  {  
      String payload = http.getString();  
      Serial.println(payload);  
  } 
  else // 访问不成功，打印原因
  {   
      String payload = http.getString();  
      Serial.print("context:");  
      Serial.println(payload);  
  }  
}  
 
/* 从服务器取得数据 
 * device为设备号，sensor为传感器号 
 */  
void readYeelinkData(String device, String sensor) 
{  
  HTTPClient http;  
  // 这里使用的api是v1.0的，v1.1的api需要提供U-ApiKey  
  const String apiAddress = "/v1.0/device/" + device + "/sensor/" + sensor + "/datapoints";  
  http.begin("api.yeelink.net", 80, apiAddress);  
  int httpCode =  http.GET(); // 使用GET形式来取得数据  
  Serial.print("code:");  
  Serial.println(httpCode);  
  if(httpCode == 200) 
  { // 访问成功，取得返回参数  
      String payload = http.getString();  
      Serial.println(payload);  
  } 
  else 
  { // 访问不成功，打印原因  
     String payload = http.getString();  
     Serial.print("context:");  
     Serial.println(payload);  
  }  
}  
void setup() 
{  
  Serial.begin ( 115200 );  
  int connectCount = 0;  
  WiFi.begin ( ssid.c_str(), password.c_str());
  
  while ( WiFi.status() != WL_CONNECTED ) 
  {  
    delay ( 1000 );  
    Serial.print ( "." );  
    if(connectCount > 30) 
    {  
      Serial.println( "Connect fail!" );  
      break;  
    }  
    connectCount += 1;  
  }  
  if(WiFi.status() == WL_CONNECTED) 
  {  
    Serial.println ( "" );  
    Serial.print ( "Connected to " );  
    Serial.println ( ssid );  
    Serial.print ( "IP address: " );  
    Serial.println ( WiFi.localIP() );  
    connectCount = 0;  
  }  
  //readYeelinkData("8938", "28887"); // 读取数据点测试  
  uploadYeelinkData("8938", "28887", "1"); // 写入数据点测试  
}  
   
void loop() 
{  
    
}
