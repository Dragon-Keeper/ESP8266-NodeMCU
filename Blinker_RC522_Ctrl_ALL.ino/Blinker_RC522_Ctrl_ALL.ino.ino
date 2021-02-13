/**
   ----------------------------------------------------------------------------
   This is a MFRC522 library example; see https://github.com/miguelbalboa/rfid
   for further details and other examples.

   NOTE: The library file MFRC522.h has a lot of useful info. Please read it.

   Released into the public domain.
   ----------------------------------------------------------------------------
   This sample shows how to read and write data blocks on a MIFARE Classic PICC
   (= card/tag).

   BEWARE: Data will be written to the PICC, in sector #1 (blocks #4 to #7).


   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15

*/

#include <SPI.h>
#include <MFRC522.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <ESP8266WiFi.h>
#include <Blinker.h>

#define RST_PIN         0           // Configurable, see typical pin layout above
#define SS_PIN          15          // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;

const char *host = "My ESP8266 Server";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
int relayInput = LED_BUILTIN;
int count = 0;
int count2 = 0;               //定义count2用于断网后重连前的延时计数
bool WIFI_Status = true;
char auth[] = "8520021e8f7c"; //你的设备key
int GPIO = 0;                 //定义GPIO 口用于控制继电器
int OTA = 0;                  //定义OTA用于控制OTA与否
int DOOR = 0;                 //定义DOOR用于控制开门与否
int ledPin = 2; //通过闪烁的快慢提示是否连上WiFi（未连上：快速闪烁；已连上：3秒一次）
#define BUTTON_1 "Light_Key1"
#define BUTTON_2 "BurnKey"
#define BUTTON_3 "DoorKey"
BlinkerButton Button1("Light_Key1"); //这里需要根据自己在BLINKER里面设置的名字进行更改
BlinkerButton Button2("BurnKey");    //这里需要根据自己在BLINKER里面设置的名字进行更改
BlinkerButton Button3("DoorKey");    //这里需要根据自己在BLINKER里面设置的名字进行更改

/**
   Helper routine to dump a byte array as hex values to Serial.（将字节数组转储为串行的十六进制值）
*/
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void smartConfig() //配网函数
{
  WiFi.mode(WIFI_STA);                           //使用wifi的STA模式
  Serial.println("\r\nWait for Smartconfig..."); //串口打印
  WiFi.beginSmartConfig();                       //等待手机端发出的名称与密码
  //死循环，等待获取到wifi名称和密码
  while (WiFi.status() != WL_CONNECTED) //当已连接网络连接时退出smartConfig
  {
    //等待过程中一秒打印一个.
    Serial.print(".");
    Serial.print(random(9));
    digitalWrite(ledPin, LOW);
    Blinker.delay(500);
    digitalWrite(ledPin, HIGH);
    Blinker.delay(500);
    if (WiFi.smartConfigDone()) //获取到之后退出等待
    {
      Serial.println("SmartConfig Success");
      //打印获取到的wifi名称和密码
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      digitalWrite(ledPin, LOW);
      Blinker.delay(500);
      digitalWrite(ledPin, HIGH);
      Blinker.delay(2000);
      break;
    }
  }
  count2 = 0; //重置count2的值为0，防止第二次断网时直接已经等于6就开始SmartConfig
  Serial.print("count2 reset:");
  Serial.println(count2);
}

void WIFI_Init()
{
  Serial.println("\r\nConnecting");
  //当设备没有联网的情况下，执行下面的操作
  while (WiFi.status() != WL_CONNECTED)
  {
    if (WIFI_Status) //WIFI_Status为真,尝试使用flash里面的信息去连接路由器
    {
      Serial.print(".");
      Blinker.delay(1000);
      count++;
      if (count >= 10)
      {
        WIFI_Status = false;
        Serial.println("WiFi connect fail,please config by phone");
        //digitalWrite(ledPin, LOW);
        //Blinker.delay(500);
        //digitalWrite(ledPin, HIGH);
        //Blinker.delay(500);
      }
    }
    else //使用flash中的信息去连接wifi失败，执行
    {
      smartConfig(); //smartConfig技术配网
    }
  }
  //串口打印连接成功的IP地址
  Serial.println("Connected");
  Serial.print("IP:");
  Serial.println(WiFi.localIP());
  //digitalWrite(ledPin, LOW);
  //Blinker.delay(500);
  //digitalWrite(ledPin, HIGH);
  //Blinker.delay(2000);
}

// 控制灯的函数
void button1_callback(const String &state)
{
  BLINKER_LOG("1#灯，状态: ", state);
  if (state == "on")
  {
    digitalWrite(GPIO, HIGH);
    Button1.icon("fal fa-lightbulb-on");
    Button1.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button1.text("已开灯");
    Button1.print("on");
    BLINKER_LOG("1#灯已开灯on"); //串口打印
  }
  else if (state == "off")
  {
    digitalWrite(GPIO, LOW);
    Button1.icon("fal fa-lightbulb");
    Button1.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button1.text("已关灯");
    Button1.print("off");
    BLINKER_LOG("1#灯已关灯off");
  }
}
// 控制OTA的函数
void button2_callback(const String &state)
{
  BLINKER_LOG("OTA，状态: ", state);
  if (state == "on")
  {
    OTA = 1;
    Button2.icon("fal fa-satellite-dish");
    Button2.color("#FF0000"); //2#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button2.text("已开OTA");
    Button2.print("on");
    BLINKER_LOG("OTA开启"); //串口打印
  }
  else if (state == "off")
  {
    OTA = 0;
    Button2.icon("fab fa-jedi-order");
    Button2.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button2.text("已关OTA");
    Button2.print("off");
    BLINKER_LOG("OTA关闭");
  }
}
// 控制门锁的函数
void button3_callback(const String &state)
{
  BLINKER_LOG("门锁，状态: ", state);
  if (state == "on")
  {
    DOOR = 1;
    Button3.icon("fad fa-door-open");
    Button3.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button3.text("已开门");
    Button3.print("on");
    BLINKER_LOG("已开门"); //串口打印
  }
  else if (state == "off")
  {
    DOOR = 0;
    Button3.icon("fad fa-door-closed");
    Button3.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button3.text("已关门");
    Button3.print("off");
    BLINKER_LOG("已关门");
  }
}

void miotPowerState(const String &state)
{
  BLINKER_LOG("need set power state: ", state);

  if (state == BLINKER_CMD_OFF)
  { //如果语音接收到是关闭灯就设置GPIO口为高电平
    digitalWrite(GPIO, HIGH);
    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_ON)
  {
    digitalWrite(GPIO, LOW);
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
}

void miotPowerState1(const String &state)
{
  BLINKER_LOG("need set OTA state: ", state);

  if (state == BLINKER_CMD_OFF)
  {
    OTA = 0;
    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_ON)
  {
    OTA = 1;
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
}

void miotPowerState2(const String &state)
{
  BLINKER_LOG("need set Door state: ", state);

  if (state == BLINKER_CMD_OFF)
  {
    DOOR = 0;
    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_ON)
  {
    DOOR = 1;
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
}

void setup() {
  Serial.begin(115200); // Initialize serial communications with the PC
  //while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(GPIO, OUTPUT);
  digitalWrite(GPIO, HIGH);                                     //初始化，由于继电器是低电平触发。所以刚开始设为高电平
  WIFI_Init();                                                  //调用WIFI函数
  Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str()); //运行blinker
  Button1.attach(button1_callback);
  Button2.attach(button2_callback);
  Button3.attach(button3_callback);
  BlinkerMIOT.attachPowerState(miotPowerState);  //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  BlinkerMIOT.attachPowerState(miotPowerState1); //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  BlinkerMIOT.attachPowerState(miotPowerState2); //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println(F("Scan a MIFARE Classic PICC to demonstrate read."));
  Serial.print(F("Using key (for A and B):"));
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
}

void loop() {
  Blinker.run();
  WIFI_Init(); //直接使用这个函数来检测断网与否以及断网后的重联

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle. & Select one of the cards
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial())
  {
    //下面return判断发现没有卡可读，让程序重新由loop开始运行，所以要屏蔽
    //return; 
  }
  else
  {
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak); //读取卡片类型
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // 根据读取的卡片类型检查兼容性
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
            &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
      Serial.println(F("This sample only works with MIFARE Classic cards."));
      return;
    }
    // 下面代码将dataBlock[]的内容写入覆盖block 4 的内容
    // In this sample we use the second sector
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;
    byte blockAddr      = 4;
    byte dataBlock[]    = {
      0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
      0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
      0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
      0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
    byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    //读取key A数据并存入status然后显示出来
    // Authenticate using key A
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    // Show the whole sector as it currently is。（显示整个扇区的数据并存入缓存）
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

    // Read data from the block
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();

    if (buffer[0] == 0x01 && buffer[1] == 0x02 && buffer[2] == 0x03 && buffer[3] == 0x04)
    { //该数据内容可以更改为自己的IC卡内第N道的对应内容等
      Serial.println("Hello Arduino!");
      //这里写入如果号码对的话如何操作
      Blinker.delay(2000);
    }
    else
    { //若卡号与上述不符
      Serial.println("Hello unkown guy!");
      //这里写入如果号码不对的话如何操作
      Blinker.delay(2000);
    }
    //-----上面到注释之间内容为判断第四道前四位内容是否相符，符合的话执行相应反馈
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  }

  if (OTA == 0)
  {
    Serial.println("We Won't Burn.");
    Blinker.delay(500);
  }
  else
  {
    /*
       下面if至while直接这里申请存储tcp设置项user_tcp_conn.proto.tcp，
      如果放进while里面循环申请堆空间，而却重来没有释放，就会堆空间溢出而重启系统
      相关链接：https://blog.csdn.net/d521000121/article/details/70196896/
    */
    if (OTA == 1)
    {
      Serial.println("Wait For Firmware To Burn.");
      WiFi.mode(WIFI_AP_STA);
      MDNS.begin(host);
      httpUpdater.setup(&httpServer);
      httpServer.begin();
      MDNS.addService("http", "tcp", 80);
      Serial.print("HTTPUpdateServer ready! Open \"http://");
      Serial.print(WiFi.localIP());
      Serial.print("/update\" in your browser\n");
      while (OTA == 1) //由上面if进入申请建立tcp服务器，这里等待固件上传并更新
      {
        /*
           http://www.taichi-maker.com/homepage/iot-development/iot-dev-reference/esp8266-c-plus-plus-reference/esp8266webserver/handleclient/
           此函数主要作用是检查有没有客户端设备通过网络向ESP8266网络服务器发送请求。
           每一次handleClient`函数被调用时，ESP8266网络服务器都会检查一下是否有客
           户端发送HTTP请求。

           假如loop函数里有类似delay一类的函数延迟程序运行，那么就一定要注意了。
           如果handleClient函数长时间得不到调用，ESP8266网络服务器会因为无法经常
           检查HTTP客户端请求而导致服务器响应变慢，严重的情况下，会导致服务器工作不稳定。

           更多相关链接：
           http://www.taichi-maker.com/homepage/esp8266-nodemcu-iot/iot-c/esp8266-nodemcu-web-server/
        */
        httpServer.handleClient(); //处理http服务器访问
        Blinker.delay(1000);
      }
    }
  }

  if (DOOR == 0)
  {
    Serial.println("DOOR Is Closed.");
    Blinker.delay(500);
  }
  else
  {
    Serial.println("DOOR Is Opened.");
    Blinker.delay(500);
  }
}
