#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <ESP8266WiFi.h>
#include <Blinker.h>

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

void setup()
{
  Serial.begin(115200);
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
}

void loop()
{
  Blinker.run();
  //---------------------这一段用于使用过程中断网后从新联网用
  /*
     下面这句用来检测网络连接是否成功，WiFi.status()获取网络状态，
     WL_CONNECTED表示网络连接成功，!= 表示不等于，就是说网络连接状态不成功
  */
  if (WiFi.status() != WL_CONNECTED) //WIFI连接失败
  {
    count2++;
    Blinker.delay(1000);
    Serial.print("count2:");
    Serial.println(count2);
    if (count2 >= 6)
    {
      //Serial.print(WiFi.status());
      //Serial.println("if 3 connected;or 6 disconnect.");
      Serial.println("WiFi fail,The light flashes every 0.5 seconds.");
      //-----------这段用板载小灯的闪速显示未连接
      smartConfig(); //smartConfig技术配网
      //digitalWrite(ledPin, LOW);
      //Blinker.delay(500);
      //digitalWrite(ledPin, HIGH);
      //Blinker.delay(500);
    }
  }
  else
  {
    //Serial.println("Connected,The light flashes every 3 seconds.");
    //Serial.print("IP:");
    //Serial.println(WiFi.localIP()); //串口打印连接成功的IP地址
    //-----------这段用板载小灯的闪速显示已连接
    //digitalWrite(ledPin, LOW);
    //Blinker.delay(500);
    //digitalWrite(ledPin, HIGH);
    //Blinker.delay(2000);
  }
  //---------------------上面这一段用于使用过程中断网后从新联网用*/
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
