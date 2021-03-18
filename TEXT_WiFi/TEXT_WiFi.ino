#define BLINKER_WIFI

//启用非加密通信，获得更大的可用ram，不然8266ram不够就会重启。
//目前仅可用于ESP8266，其他设备的RAM足以进行加密通信
#define BLINKER_WITHOUT_SSL 

#include <Blinker.h>

char auth[] = "f5f144f4ede6";
char ssid[] = "MERCURY_B8A4";
char pswd[] = "3133xjzx100";

#define TEXTE_1 "TextKey"
BlinkerText Text1(TEXTE_1);

void dataRead(const String & data)
{
  Text1.print(WiFi.localIP().toString(),"/update");
}

void setup()
{
  Serial.begin(115200);
  BLINKER_DEBUG.stream(Serial);
  Blinker.begin(auth, ssid, pswd);
  Blinker.attachData(dataRead);
}

void loop()
{
  Blinker.run();
  blinker.delay(100);
}
