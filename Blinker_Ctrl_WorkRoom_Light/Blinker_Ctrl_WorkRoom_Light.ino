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

char auth[] = "62802af66f0d"; //你的设备key
//int ledPin = 2 ;            //通过闪烁的快慢提示是否连上WiFi（未连上：快速闪烁；已连上：3秒一次）
int GPIO = 0; //定义GPIO0用于控制继电器
int OTA = 0;
int Light = 0;    //定义Light用于判断开灯与否
int count2 = 120; //定义count2用于计算等待SmartConfig超时重启
#define BUTTON_1 "Light_Key1"
#define BUTTON_2 "BurnKey"
#define TEXTE_1 "TextKey"
BlinkerText Text1(TEXTE_1);
BlinkerButton Button1("Light_Key1"); //这里需要根据自己在BLINKER里面设置的名字进行更改
BlinkerButton Button2("BurnKey");    //这里需要根据自己在BLINKER里面设置的名字进行更改

//新建数据类型组件对象，作用：将数据传输到手机blinker app
BlinkerNumber FreeH("FreeHeap"); //定义内存数据键名
BlinkerNumber LiveT("W_L_LiveTime");
float W_L_LiveTime = 0; //定义W_L_LiveTime用于计算生存时间

//-----------下面这段用于ESP-NOW局域网接收信息通讯用------//
// Structure example to receive data
// Must match the receiver structure
typedef struct struct_message
{
  int b;
} struct_message;

// Create a struct_message called myData
struct_message myData;
//-----------上面这段用于ESP-NOW局域网接收信息通讯用------//

const char *host = "My ESP8266 Server";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void smartConfig() //配网函数
{
  WiFi.mode(WIFI_STA);     //使用wifi的STA模式
  WiFi.beginSmartConfig(); //开启SmartConfig
  //Serial.println("\r\nWait for Smartconfig...");
  while (!WiFi.smartConfigDone()) //等待手机端发出的名称与密码
  {
    //digitalWrite(ledPin, LOW);
    Serial.println(".");
    //Serial.println(ESP.getFreeHeap()); //获取可用堆大小
    //Serial.println("SmartConfiging");
    Blinker.delay(200);
    //digitalWrite(ledPin, HIGH);
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
    //digitalWrite(ledPin, LOW);
    Blinker.delay(800);
    //digitalWrite(ledPin, HIGH);
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
      //digitalWrite(ledPin, LOW);
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
    digitalWrite(GPIO, LOW);
    Button1.icon("fal fa-lightbulb-on");
    Button1.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button1.text("已开灯");
    Button1.print("on");
    //BLINKER_LOG("1#灯已开灯on"); //串口打印
  }
  else if (state == "off")
  {
    digitalWrite(GPIO, HIGH);
    Button1.icon("fal fa-lightbulb");
    Button1.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button1.text("已关灯");
    Button1.print("off");
    //BLINKER_LOG("1#灯已关灯off");
    Serial.println(ESP.getFreeHeap());
    Serial.println("Light");
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
    /*
      下面if至while直接这里申请存储tcp设置项user_tcp_conn.proto.tcp，
      如果放进while里面循环申请堆空间，而却重来没有释放，就会堆空间溢出而重启系统
      相关链接：https://blog.csdn.net/d521000121/article/details/70196896/

      WiFi.mode(WIFI_STA);
      MDNS.begin(host);
      httpUpdater.setup(&httpServer);
      httpServer.begin();
      MDNS.addService("http", "tcp", 80);
    */
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
      MDNS.update();
      Blinker.delay(1000);
      Serial.println(ESP.getFreeHeap());
      Serial.println("OTA");
    }
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

//上传剩余内存数据
void heartbeat()
{
  //FreeH.print(ESP.getFreeHeap()); //给blinkerapp回传内存数据
  LiveT.print(W_L_LiveTime);          //上传生存时间计数，重启时重新计数，由数据曲线可知重启与否
}

void dataStorage() //在回调函数中，设定要存储的键名和值，让Blinker储存内存数据到服务器
{
  //Blinker.dataStorage("FreeHeap", ESP.getFreeHeap()); //添加数据存储以便于图标数据展示
  Blinker.dataStorage("W_L_LiveTime", W_L_LiveTime);          //添加数据存储以便于图标数据展示
}

// ESP-NOW局域网通讯接收用
// Callback function that will be executed when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
  memcpy(&myData, incomingData, sizeof(myData));
  Light = myData.b;
  if (Light == 1)
  {
    Serial.println("Light Is On.");
    digitalWrite(GPIO, LOW);
    Button1.icon("fal fa-lightbulb-on");
    Button1.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button1.text("已开灯");
    Button1.print("on");
    //BLINKER_LOG("1#灯已开灯on"); //串口打印
    Serial.println(ESP.getFreeHeap());
    Serial.println("Light Is Open.");
  }
  else if (Light == 0)
  {
    Serial.println("Light Is Off.");
    digitalWrite(GPIO, HIGH);
    Button1.icon("fal fa-lightbulb");
    Button1.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button1.text("已关灯");
    Button1.print("off");
    //BLINKER_LOG("1#灯已关灯off");
    Serial.println(ESP.getFreeHeap());
    Serial.println("Light Is Close.");
  }
}

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC
  //BLINKER_DEBUG.stream(Serial); //开了这个就显示MQTT信息
  WiFi.mode(WIFI_AP_STA);
  Serial.println(WiFi.macAddress());
  pinMode(GPIO, OUTPUT);
  digitalWrite(GPIO, HIGH); //初始化，由于继电器是高电平触发。所以刚开始设为低电平
  Blinker.attachData(dataRead);
  Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str()); //运行blinker
  Button1.attach(button1_callback);
  Button2.attach(button2_callback);
  BlinkerMIOT.attachPowerState(miotPowerState);  //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  BlinkerMIOT.attachPowerState(miotPowerState1); //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  Blinker.attachHeartbeat(heartbeat);            //将传感器获取的数据传给blinker app上
  Blinker.attachDataStorage(dataStorage);        //关联回调函数，开启历史数据存储功能

  /*
    下面4行代码直接申请存储tcp设置项user_tcp_conn.proto.tcp，
    如果放进循环代码中申请堆空间，而却重来没有释放，就会堆空间溢出而重启系统，
    所以放到这里只申请一次。
    相关链接：https://blog.csdn.net/d521000121/article/details/70196896/
  */
  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);

  if (esp_now_init() != 0)
  {
    //Serial.println("Error initializing ESP-NOW");
  }
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  Blinker.run();
  WIFI_Init(); //直接使用这个函数来检测断网与否以及断网后的重联
  //Serial.println(ESP.getFreeHeap()); //获取可用堆大小
  W_L_LiveTime = W_L_LiveTime + 1;
  //串口输出数据和Blinker.print下一句要加上Blinker.delay(33);
  //加上延时解决串口打印导致的EXCEPTION (29)(28)错误导致复位重启问题

  Blinker.delay(1000);
}
