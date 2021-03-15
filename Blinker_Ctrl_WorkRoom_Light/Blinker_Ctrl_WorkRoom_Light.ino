#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <ESP8266WiFi.h>
#include <Blinker.h>
#include <espnow.h>

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  int b;
} struct_message;

// Create a struct_message called myData
struct_message myData;

int Light = 0;                //定义Light用于判断开灯与否

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("myData.b Is:");
  Serial.println(myData.b);
  Light = myData.b;
  Serial.print("Light Is:");
  Serial.println(Light);
  Serial.println();
}

const char *host = "My ESP8266 Server";
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

char auth[] = "ee69c8793143"; //你的设备key
int relayInput = LED_BUILTIN; //LED_BUILTIN D4
int ledPin = 2 ;              //通过闪烁的快慢提示是否连上WiFi（未连上：快速闪烁；已连上：3秒一次）
int GPIO = D0;                 //定义GPIO0用于控制继电器
int OTA = 0;
#define BUTTON_1 "Light_Key1"
#define BUTTON_2 "BurnKey"
BlinkerButton Button1("Light_Key1"); //这里需要根据自己在BLINKER里面设置的名字进行更改
BlinkerButton Button2("BurnKey");    //这里需要根据自己在BLINKER里面设置的名字进行更改

void smartConfig() //配网函数
{
  WiFi.mode(WIFI_STA);     //使用wifi的STA模式
  WiFi.beginSmartConfig(); //开启SmartConfig
  Serial.println("\r\nWait for Smartconfig...");
  while (!WiFi.smartConfigDone()) //等待手机端发出的名称与密码
  {
    digitalWrite(ledPin, LOW);
    Serial.print(random(2));
    Serial.print(".");
    Serial.println("SmartConfiging");
    Blinker.delay(200);
    digitalWrite(ledPin, HIGH);
    Blinker.delay(100);
  }
  //当连接状态为未连接时，将获取到wifi名称和密码用于连接
  while (WiFi.status() != WL_CONNECTED)
  {
    //等待过程中一秒打印一个.
    Serial.print(random(9));
    Serial.print(".");
    Serial.println("Connecting By SmartConfig");
    digitalWrite(ledPin, LOW);
    Blinker.delay(800);
    digitalWrite(ledPin, HIGH);
    Blinker.delay(100);
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void WIFI_Init()
{
  Serial.print("\r\nIP Address: ");
  Serial.println(WiFi.localIP());
  //当设备没有联网的情况下，执行下面的操作
  if (WiFi.status() != WL_CONNECTED) //如果没有连接上网络
  {
    WiFi.begin(); //使用flash中的信息去连接wifi
    //延时20秒使用flash中的信息去连接wifi
    for (int count = 20; count > 0; count = count - 1)
    {
      Serial.printf("Wait For Connecting %u \n", count);
      digitalWrite(ledPin, LOW);
      Blinker.delay(1000);
    }
    Serial.println(WiFi.localIP());
    if (WiFi.status() != WL_CONNECTED) //如果还是没有连接上网络
    {
      smartConfig(); //执行smartConfig配网
    }
  }
}

// 控制灯的函数
void button1_callback(const String &state)
{
  BLINKER_LOG("1#灯，状态: ", state);
  if (state == "on")
  {
    Light = 1;
    digitalWrite(GPIO, LOW);
    Button1.icon("fal fa-lightbulb-on");
    Button1.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button1.text("已开灯");
    Button1.print("on");
    BLINKER_LOG("1#灯已开灯on"); //串口打印
  }
  else if (state == "off")
  {
    Light = 0;
    digitalWrite(GPIO, HIGH);
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

void miotPowerState(const String &state)
{
  BLINKER_LOG("need set power state: ", state);

  if (state == BLINKER_CMD_OFF)
  { //如果语音接收到是关闭灯就设置GPIO口为高电平
    digitalWrite(GPIO, LOW);
    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();
  }
  else if (state == BLINKER_CMD_ON)
  {
    digitalWrite(GPIO, HIGH);
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();
  }
}

void miotPowerState1(const String &state)
{
  BLINKER_LOG("need set OTA state: ", state);

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

void setup()
{
  Serial.begin(115200); // Initialize serial communications with the PC
  WiFi.mode(WIFI_STA);
  Serial.print("ESP8266 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(GPIO, OUTPUT);
  digitalWrite(GPIO, HIGH);  //初始化，由于继电器是高电平触发。所以刚开始设为低电平
  Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str()); //运行blinker
  Button1.attach(button1_callback);
  Button2.attach(button2_callback);
  BlinkerMIOT.attachPowerState(miotPowerState);  //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  BlinkerMIOT.attachPowerState(miotPowerState1); //这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
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
  Serial.print("Light Is:");
  Serial.println(Light);
  if (Light == 0)
  {
    Serial.println("Light Is Close.");
    digitalWrite(GPIO, HIGH);
    Button1.icon("fal fa-lightbulb");
    Button1.color("#000000"); //2#按钮没有按下时，app按键颜色状态显示是黑色
    // 反馈开关状态
    Button1.text("已关灯");
    Button1.print("off");
    BLINKER_LOG("1#灯已关灯off");
  }
  else
  {
    Serial.println("Light Is On.");
    digitalWrite(GPIO, LOW);
    Button1.icon("fal fa-lightbulb-on");
    Button1.color("#FF0000"); //1#按钮按下时，app按键颜色状态显示是红色
    // 反馈开关状态
    Button1.text("已开灯");
    Button1.print("on");
    BLINKER_LOG("1#灯已开灯on"); //串口打印
  }

  if (OTA == 0)
  {
    Serial.println("We Won't Burn.");
    Blinker.delay(200);
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
}
