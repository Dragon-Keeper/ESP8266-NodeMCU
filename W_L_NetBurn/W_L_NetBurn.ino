#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <ESP8266WiFi.h>
#include <Blinker.h>

// char auth[] = "8520021e8f7c";  //设备key，打开点灯app，添加设备，选择arduino设备，选择wifi接入，这里就会获得一个密钥，把密钥填写在这里。
// char ssid[] = "DarthVader";  //路由器wifi ssid
// char pswd[] = "aa123456";  //路由器wifi 密码

int count = 0;
bool WIFI_Status = true;
char auth[] = "8520021e8f7c";  //你的设备key
int GPIO = 0;//定义GPIO口用于控制继电器
int ledPin = 2; // GPIO2 of ESP8266-01S

#define BUTTON_1 "ButtonKey"

BlinkerButton Button1("Light_key1");//这里需要根据自己在BLINKER里面设置的名字进行更改

void smartConfig()//配网函数
{
  WiFi.mode(WIFI_STA);//使用wifi的STA模式
  Serial.println("\r\nWait for Smartconfig...");//串口打印
  WiFi.beginSmartConfig();//等待手机端发出的名称与密码
  //死循环，等待获取到wifi名称和密码
  while (1)
  {
    //等待过程中一秒打印一个.
    Serial.print(".");
    delay(1000);                                             
    if (WiFi.smartConfigDone())//获取到之后退出等待
    {
      Serial.println("SmartConfig Success");
      //打印获取到的wifi名称和密码
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      break;
    }
  }
}

void WIFI_Init()
{
    Serial.println("\r\n正在连接");
    //当设备没有联网的情况下，执行下面的操作
    while(WiFi.status()!=WL_CONNECTED)
    {
        if(WIFI_Status)//WIFI_Status为真,尝试使用flash里面的信息去连接路由器
        {
            Serial.print(".");
            delay(1000);                                        
            count++;
            if(count>=5)
            {
                WIFI_Status = false;
                Serial.println("WiFi连接失败，请用手机进行配网"); 
            }
        }
        else//使用flash中的信息去连接wifi失败，执行
        {
            smartConfig();  //smartConfig技术配网
        }
     }  
     //串口打印连接成功的IP地址
     Serial.println("连接成功");  
     Serial.print("IP:");
     Serial.println(WiFi.localIP());
}

void button1_callback(const String & state)
{
    BLINKER_LOG("get button state: ", state);
    digitalWrite(GPIO,!digitalRead(GPIO));
    Blinker.vibrate(); 
}


void miotPowerState(const String & state)
{
    BLINKER_LOG("need set power state: ",state);

    if (state == BLINKER_CMD_OFF) {//如果语音接收到是关闭灯就设置GPIO口为高电平
        digitalWrite(GPIO, HIGH);

        BlinkerMIOT.powerState("off");
        BlinkerMIOT.print();
    }
    else if (state == BLINKER_CMD_ON) {
        digitalWrite(GPIO, LOW);
        BlinkerMIOT.powerState("on");
        BlinkerMIOT.print();
    }
}


void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    pinMode(GPIO,OUTPUT);
    digitalWrite(GPIO,HIGH);//初始化，由于继电器是低电平触发。所以刚开始设为高电平
    // Blinker.begin(auth, ssid, pswd);
    WIFI_Init();//调用WIFI函数
    Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());//运行blinker
    Button1.attach(button1_callback);
    BlinkerMIOT.attachPowerState(miotPowerState);//这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态


}

void loop()
{
    Blinker.run();
    digitalWrite(ledPin, HIGH);
    delay(1000);           
    digitalWrite(ledPin, LOW);
    delay(1000);  
}



