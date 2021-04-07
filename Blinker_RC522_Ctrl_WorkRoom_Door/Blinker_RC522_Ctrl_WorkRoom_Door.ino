/* Typical pin layout used:
  -----------------------------------------------------------------------------------------
              MFRC522      Arduino    ESP8266     Arduino    Arduino          Arduino
              Reader/PCD   Uno/101  Wemos D1 mini  Nano v3    Leonardo/Micro   Pro Micro
  Signal      Pin          Pin           Pin       Pin        Pin              Pin
  -----------------------------------------------------------------------------------------
  RST/Reset   RST          9             D1 / 5    D9         RESET/ICSP-5     RST
  SPI SS      SDA(SS)      10            D3 / 0    D10        10               10
  SPI MOSI    MOSI         11 / ICSP-4   D7 / 13   D11        ICSP-4           16
  SPI MISO    MISO         12 / ICSP-1   D6 / 12   D12        ICSP-1           14
  SPI SCK     SCK          13 / ICSP-3   D5 / 14   D13        ICSP-3           15
  SPI IRQ     IRQ          7             D  /
  1. 时钟(SPI CLK, SCLK)
  2. 片选(CS)
  3. 主机输出、从机输入(MOSI)
  4. 主机输入、从机输出(MISO) 产生时钟信号的器件称为主机。
  5. IRQ中断才用到的，没有用到可以不接。
    -----------------------------------------------------------------------------------------

              LCD1602     NodeMCU 8266 LoLin V3
  Signal      Pin          Pin
  -------------------------------------
  SPI SS      SDA         D2
  SPI SCL     SCL         D1
  -----------------------------------------------------------------------------------------

              1306OLED     NodeMCU 8266 LoLin V3
  Signal      Pin          Pin
  -------------------------------------
  SPI SS      SDA         D2
  SPI SCL     SCL         D3
  -----------------------------------------------------------------------------------------
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
#include <espnow.h>


//启用非加密通信，获得更大的可用ram，不然8266ram不够就会重启。
//目前仅可用于ESP8266，其他设备的RAM足以进行加密通信
#define BLINKER_WITHOUT_SSL

//-----------下面这段用于ESP-NOW局域网发生信息通讯用------//
// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xC8, 0x2B, 0x96, 0x2E, 0x9D, 0x89};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{
  int b;
} struct_message;

// Create a struct_message called myData
struct_message myData;

unsigned long lastTime = 0;
unsigned long timerDelay = 2000; // send readings timer
//-----------上面这段用于ESP-NOW局域网发生信息通讯用------//

#define RST_PIN D1                // Configurable, see typical pin layout above
#define SS_PIN D3                 // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.
MFRC522::MIFARE_Key key;

const char *host = "My ESP8266 Server";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

char auth[] = "8520021e8f7c"; //你的设备key
int relayInput = LED_BUILTIN; //LED_BUILTIN D4
int ledPin = 2;               //通过闪烁的快慢提示是否连上WiFi（未连上：快速闪烁；已连上：3秒一次）
int Light = 0;                //定义Light用于判断开灯与否
int count2 = 120;             //定义count2用于计算等待SmartConfig超时重启
int OTA = 0;                  //定义OTA用于控制OTA与否
int DOOR = 0;                 //定义DOOR用于控制开门与否
int NFC = 0;                  //定义NFC用于判断NFC工作与否
int servoPin = D8;            //定义GPIO15 D8用于控制舵机
int D_Button = D2;            //定义GPIO4 D2用于触摸开关控制舵机开门
int L_Button = D0;            //定义GPIO16 D0用于触摸开关控制开关灯
//int ButtonState = 1;          //用于获取灯触摸开关的状态
//int ButtonLastState = 1;      //用于保持灯触摸开关的状态并比对
//int ButtonCounter = 1；       //用于整除求余改变触摸开关的状态
// 8266设备电压不够时，输入端口的IO值会在0和1之间跳转
#define BUTTON_1 "Light_Key1"
#define BUTTON_2 "BurnKey"
#define BUTTON_3 "DoorKey"
#define TEXTE_1 "TextKey"
                    BlinkerText Text1(TEXTE_1);
BlinkerButton Button1("Light_Key1"); //这里需要根据自己在BLINKER里面设置的名字进行更改
BlinkerButton Button2("BurnKey");    //这里需要根据自己在BLINKER里面设置的名字进行更改
BlinkerButton Button3("DoorKey");    //这里需要根据自己在BLINKER里面设置的名字进行更改

//新建数据类型组件对象，作用：将数据传输到手机blinker app
BlinkerNumber FreeH("FreeHeap"); //定义内存数据键名
BlinkerNumber LiveT("W_D_LiveTime");
float W_D_LiveTime = 0; //定义W_D_LiveTime用于计算生存时间

// ESP-NOW局域网通讯发送用
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
  //Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0)
  {
    //Serial.println("Delivery success");
  }
  else
  {
    //Serial.println("Delivery fail");
  }
}

/**
   Helper routine to dump a byte array as hex values to Serial.
   将字节数组转储为串行的十六进制值，用于存储读取的NFC卡数据
*/
void dump_byte_array(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    //Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    //Serial.print(buffer[i], HEX);
  }
}

void smartConfig() //配网函数
{
  WiFi.mode(WIFI_STA);     //使用wifi的STA模式
  WiFi.beginSmartConfig(); //开启SmartConfig
  //Serial.println("\r\nWait for Smartconfig...");
  while (!WiFi.smartConfigDone()) //等待手机端发出的名称与密码
  {
    digitalWrite(ledPin, LOW);
    Serial.println(".");
    //Serial.println(ESP.getFreeHeap()); //获取可用堆大小
    //Serial.println("SmartConfiging");
    Blinker.delay(200);
    digitalWrite(ledPin, HIGH);
    Blinker.delay(100);
    if (count2 < 1) //等待120S超时重启
    {
      ESP.restart();
    }
    else
    {
      count2 = count2 - 1;
      Serial.println(count2);
    }
  }
  //当连接状态为未连接时，将获取到wifi名称和密码用于连接
  while (WiFi.status() != WL_CONNECTED)
  {
    //等待过程中一秒打印一个.
    Serial.println("...");
    //Serial.println(ESP.getFreeHeap()); //获取可用堆大小
    //Serial.println("Connecting By SmartConfig");
    digitalWrite(ledPin, LOW);
    Blinker.delay(800);
    digitalWrite(ledPin, HIGH);
    Blinker.delay(100);
  }
  //Serial.print("IP Address: ");
  //Serial.println(WiFi.localIP());
}

void WIFI_Init()
{
  //Serial.print("\r\nIP Address: ");
  //Serial.println(WiFi.localIP());
  //当设备没有联网的情况下，执行下面的操作
  if (WiFi.status() != WL_CONNECTED) //如果没有连接上网络
  {
    WiFi.begin(); //使用flash中的信息去连接wifi
    //延时20秒使用flash中的信息去连接wifi
    for (int count = 20; count > 0; count = count - 1)
    {
      //Serial.printf("Wait For Connecting %u \n", count);
      digitalWrite(ledPin, LOW);
      Blinker.delay(1000);
    }
    //Serial.println(WiFi.localIP());
    if (WiFi.status() != WL_CONNECTED) //如果还是没有连接上网络
    {
      smartConfig(); //执行smartConfig配网
    }
  }
}

void dataRead(const String &data)
{
  //Blinker.print("Blinker readString:", data);
  Text1.print(WiFi.localIP().toString(), "/update");
  Serial.println(ESP.getFreeHeap());
  Serial.println("Updata IP.");
}

// 控制灯的函数
void button1_callback(const String &state)
{
  //BLINKER_LOG("1#灯，状态: ", state);
  if (state == "on")
  {
    Light = 1;
    myData.b = Light;
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    Button1.icon("fal fa-lightbulb-on");
    Button1.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button1.text("已开灯");
    Button1.print("on");
    //BLINKER_LOG("1#灯已开灯on"); //串口打印
  }
  else if (state == "off")
  {
    Light = 0;
    myData.b = Light;
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    Button1.icon("fal fa-lightbulb");
    Button1.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button1.text("已关灯");
    Button1.print("off");
    //BLINKER_LOG("1#灯已关灯off");
  }
}

// 控制OTA的函数
void button2_callback(const String &state)
{
  //BLINKER_LOG("OTA，状态: ", state);
  if (state == "on")
  {
    OTA = 1;
    Button2.icon("fal fa-satellite-dish");
    Button2.color("#FF0000"); //2#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button2.text("已开OTA");
    Button2.print("on");
    //BLINKER_LOG("OTA开启"); //串口打印
  }
  else if (state == "off")
  {
    OTA = 0;
    Button2.icon("fab fa-jedi-order");
    Button2.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button2.text("已关OTA");
    Button2.print("off");
    //BLINKER_LOG("OTA关闭");
    ESP.wdtFeed(); //释放内存（喂狗）
  }
}
// 控制门锁的函数
void button3_callback(const String &state)
{
  //BLINKER_LOG("门锁，状态: ", state);
  if (state == "on")
  {
    DOOR = 1;
    Button3.icon("fad fa-door-open");
    Button3.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button3.text("已开门");
    Button3.print("on");
    //BLINKER_LOG("已开门"); //串口打印
  }
  else if (state == "off")
  {
    DOOR = 0;
    Button3.icon("fad fa-door-closed");
    Button3.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button3.text("已关门");
    Button3.print("off");
    //BLINKER_LOG("已关门");
  }
}

void miotPowerState(const String &state)
{
  //BLINKER_LOG("need set power state: ", state);

  if (state == BLINKER_CMD_OFF)
  {
    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_ON)
  {
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
}

void miotPowerState1(const String &state)
{
  //BLINKER_LOG("need set OTA state: ", state);

  if (state == BLINKER_CMD_OFF)
  {
    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_ON)
  {
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
}

void miotPowerState2(const String &state)
{
  //BLINKER_LOG("need set Door state: ", state);

  if (state == BLINKER_CMD_OFF)
  {
    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_ON)
  {
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
}

//上传剩余内存数据
void heartbeat()
{
  //FreeH.print(ESP.getFreeHeap()); //给blinkerapp回传内存数据
  LiveT.print(W_D_LiveTime); //上传生存时间计数，重启时重新计数，由数据曲线可知重启与否
}

void dataStorage() //在回调函数中，设定要存储的键名和值，让Blinker储存内存数据到服务器
{
  //Blinker.dataStorage("FreeHeap", ESP.getFreeHeap()); //添加数据存储以便于图标数据展示
  Blinker.dataStorage("W_D_LiveTime", W_D_LiveTime); //添加数据存储以便于图标数据展示
}

void servo(int angle)
{ //定义一个脉冲函数用PWM方式控制舵机转动
  //发送50个脉冲
  for (int i = 0; i < 50; i++)
  {
    int pulsewidth = (angle * 11) + 500; //将角度转化为500-2480的脉宽值
    digitalWrite(servoPin, HIGH);        //将舵机接口电平至高
    delayMicroseconds(pulsewidth);       //延时脉宽值的微秒数
    digitalWrite(servoPin, LOW);         //将舵机接口电平至低
    delayMicroseconds(20000 - pulsewidth);
  }
  Blinker.delay(100);
}

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC
  //BLINKER_DEBUG.stream(Serial); //开了这个就显示MQTT信息
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  /*GPIO 电平状态是怎样的？
    除了 XPD_DCDC，GPIO 可以配置上拉。关于 GPIO 的上电 IO 口默认状态为：
    除了 SDIO 6根线 +GPIO4+GPIO5+GPIO16 上电 IO 默认无上拉，
    其他的 GPIO 口均有上拉。由于是内部配置上拉，所以如需下拉，需外部加下拉方式或者加一个三级管的反相电路。
    注意：GPIO 不能到 5V。GPIO4/5 外接 1M 电阻不能上拉到高电平；需 100K 电阻。
  */
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(D_Button, INPUT);//初始化开门触摸开关端口为高电平
  pinMode(L_Button, INPUT_PULLUP);
  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, LOW); //初始化舵机输出端口为低电平
  Blinker.attachData(dataRead);
  Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str()); //运行blinker
  Button1.attach(button1_callback);
  Button2.attach(button2_callback);
  Button3.attach(button3_callback);
  BlinkerMIOT.attachPowerState(miotPowerState);  //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  BlinkerMIOT.attachPowerState(miotPowerState1); //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  BlinkerMIOT.attachPowerState(miotPowerState2); //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  Blinker.attachHeartbeat(heartbeat);            //将传感器获取的数据传给blinker app上
  Blinker.attachDataStorage(dataStorage);        //关联回调函数，开启历史数据存储功能

  // Init ESP-NOW
  if (esp_now_init() != 0)
  {
    //Serial.println("Error initializing ESP-NOW");
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
}

void loop()
{
  Blinker.run();
  WIFI_Init(); //直接使用这个函数来检测断网与否以及断网后的重联
  //Serial.println(ESP.getFreeHeap()); //获取可用堆大小
  W_D_LiveTime = W_D_LiveTime + 1;
  //串口输出数据和Blinker.print下一句要加上Blinker.delay(33);
  //加上延时解决串口打印导致的EXCEPTION (29)(28)错误导致复位重启问题
  Blinker.delay(200);
  //---------------下面用一个if语句来判断RC522代码是否运行，节省CPU时间
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle. & Select one of the cards
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    //Serial.println("No Card Found.");
  }
  else
  {
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak); //读取卡片类型
    // 根据读取的卡片类型检查兼容性
    if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && piccType != MFRC522::PICC_TYPE_MIFARE_1K && piccType != MFRC522::PICC_TYPE_MIFARE_4K)
    {
      return;
    }
    // 下面代码将dataBlock[]的内容写入覆盖block 4 的内容
    // In this sample we use the second sector
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector = 1;
    byte blockAddr = 4;
    byte dataBlock[] = {
      0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
      0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
      0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
      0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };
    byte trailerBlock = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    //读取key A数据并存入status然后显示出来
    status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
      return;
    }
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK)
    {
    }
    dump_byte_array(buffer, 16);

    if (buffer[0] == 0x01 && buffer[1] == 0x02 && buffer[2] == 0x03 && buffer[3] == 0x04)
    { //该数据内容可以更改为自己的IC卡内第N道的对应内容等
      Serial.println("My Lord! Door is Open.");
      //这里写入如果号码对的话如何操作--可以用来放按键开门代码----------
      NFC = 1;
      DOOR = 1;
      //Serial.print("Now The Time is:");
      //Serial.println(hour);
      int8_t hour = Blinker.hour();
      if (hour >= 7 & hour <= 18)
      {
        Light = 0;
      }
      else
      {
        Light = 1;
        myData.b = Light;
        // Send message via ESP-NOW
        esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
      }
      Blinker.delay(100);
    }
    else
    { //若卡号与上述不符
      Serial.println("Hello Unkown Guy!");
      //这里写入如果号码不对的话如何操作
      NFC = 0;
      DOOR = 0;
      Blinker.delay(100);
    }
    //-----上面到注释之间内容为判断第四道前四位内容是否相符，符合的话执行相应反馈
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();
  }
  //------------------------------if语句判断RC522工作与否到这里结束

  Blinker.delay(200);
  if (OTA == 0)
  {
    //Serial.println("We Won't Burn.");
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
      //Serial.println("Wait For Firmware To Burn.");
      WiFi.mode(WIFI_AP_STA);
      MDNS.begin(host);
      httpUpdater.setup(&httpServer);
      httpServer.begin();
      MDNS.addService("http", "tcp", 80);
      //Serial.print("HTTPUpdateServer ready! Open \"http://");
      //Serial.print(WiFi.localIP());
      //Serial.print("/update\" in your browser\n");
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

          目前就是因为这个原因导致8266重启！！！尝试一下打开OTA后登陆入服务器，看多久重启。

          更多相关链接：
          http://www.taichi-maker.com/homepage/esp8266-nodemcu-iot/iot-c/esp8266-nodemcu-web-server/
        */
        //添加计数语句，超时用break退出while循环。
        httpServer.handleClient(); //处理http服务器访问
        Blinker.delay(1000);
      }
    }
  }

  Blinker.delay(200);
  if (DOOR == 1)
  {
    //Serial.println("DOOR Is Opened.");
    Blinker.delay(500);
    Button3.icon("fad fa-door-open");
    Button3.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button3.text("已开门");
    Button3.print("on");
    //BLINKER_LOG("已开门"); //串口打印
    servo(95);           //调用函数传值直接转90度
    Blinker.delay(5000); //延时开门动作5秒
    servo(5);            //调用函数传值复位为5度
    Button3.icon("fad fa-door-closed");
    Button3.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button3.text("已关门");
    Button3.print("off");
    //BLINKER_LOG("已关门");
    DOOR = 0;
    NFC = 0;
  }

  Blinker.delay(200);
  if (digitalRead(D_Button) == 0) //触摸开关时低电平，值为0则开门
  {
    DOOR = 1; //改变赋值启动开门代码，开门后自动更改DOOR值为0
  }

  Blinker.delay(200);
  /* 自锁开关控制灯，但是控制逻辑还没想通
    ButtonState = digitalRead(L_Button);
    if (ButtonState != ButtonLastState)
    {
    if (ButtonState == 0)
    {
      Light = 1;
      myData.b = Light;
      // Send message via ESP-NOW
      esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    }
    else if (ButtonState == 1)
    {
      Light = 0;
      myData.b = Light;
      // Send message via ESP-NOW
      esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    }
    }
    ButtonLastState = ButtonState;
  */

  if (digitalRead(L_Button) == 0) //D0默认高电平，所以触摸开关时低电平，值为0则告诉灯控模块关灯
  {
    Light = 0;
    myData.b = Light;
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    Button1.icon("fal fa-lightbulb");
    Button1.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button1.text("已关灯");
    Button1.print("off");
    //BLINKER_LOG("1#灯已关灯off");
  }

}
