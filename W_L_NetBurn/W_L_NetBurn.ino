#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <ESP8266WiFi.h>
#include <Blinker.h>


int count = 0;
bool WIFI_Status = true;
char auth[] = "8520021e8f7c";  //你的设备key
int GPIO = 0;//定义GPIO口用于控制继电器
int GPIO1 = 2;//定义GPIO1口用于控制是否开启烧录模式
int f = 0;
int flag = 0;
int ledPin = 2; //通过闪烁的快慢提示是否连上WiFi（未连上：快速闪烁；已连上：3秒一次）
#define BUTTON_1 "Light_Key1"
#define BUTTON_2 "BurnKey"

BlinkerButton Button1("Light_Key1");//这里需要根据自己在BLINKER里面设置的名字进行更改
BlinkerButton Button2("BurnKey");//这里需要根据自己在BLINKER里面设置的名字进行更改

void smartConfig()//配网函数
{
  WiFi.mode(WIFI_AP_STA);//使用wifi的AP_STA模式
  Serial.println("\r\nWait for Smartconfig...");//串口打印
  WiFi.beginSmartConfig();//等待手机端发出的名称与密码
  //死循环，等待获取到wifi名称和密码
  while (1)
  {
    //等待过程中一秒打印一个.
    Serial.print(".");
    Serial.print(random(9));
    digitalWrite(ledPin, LOW);
    Blinker.delay(500);           
    digitalWrite(ledPin, HIGH);
    Blinker.delay(500); 
    Blinker.delay(1000);                                             
    if (WiFi.smartConfigDone())//获取到之后退出等待
    {
      Serial.println("SmartConfig Success");
      //打印获取到的wifi名称和密码
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      digitalWrite(ledPin, LOW);
      Blinker.delay(1000);           
      digitalWrite(ledPin, HIGH);
      Blinker.delay(3000); 
      break;
    }
  }
}

void WIFI_Init()
{
    Serial.println("\r\nConnecting");
    //当设备没有联网的情况下，执行下面的操作
    while(WiFi.status()!=WL_CONNECTED)
    {
        if(WIFI_Status)//WIFI_Status为真,尝试使用flash里面的信息去连接路由器
        {
            Serial.print(".");
            Blinker.delay(1000);                                        
            count++;
            if(count>=5)
            {
                WIFI_Status = false;
                Serial.println("WiFi connect fail,please config by phone"); 
                digitalWrite(ledPin, LOW);
                Blinker.delay(1000);           
                digitalWrite(ledPin, HIGH);
                Blinker.delay(1000);  
            }
        }
        else//使用flash中的信息去连接wifi失败，执行
        {
            smartConfig();  //smartConfig技术配网
        }
     }  
     //串口打印连接成功的IP地址
     Serial.println("Connected");  
     Serial.print("IP:");
     Serial.println(WiFi.localIP());
     digitalWrite(ledPin, LOW);
     Blinker.delay(1000);           
     digitalWrite(ledPin, HIGH);
     Blinker.delay(3000);  
}

void button1_callback(const String & state)
{
    BLINKER_LOG("get Light_button state: ", state);
    Serial.print("get Light_button state: ");
    Serial.println(state);
    digitalWrite(GPIO,!digitalRead(GPIO));
    int m = digitalRead(GPIO);
    Blinker.vibrate(); //震动反馈
    if(m == HIGH)
    {
      f = 1;
      Serial.println(f);
    }
    else
    {
      f = 0;
      Serial.println(f);
    }
}

void button2_callback(const String & state)
{
    BLINKER_LOG("get OTA_button state: ", state);
    Serial.print("get OTA_button state: ");
    Serial.println(state);
    digitalWrite(GPIO1,!digitalRead(GPIO1));
    int n = digitalRead(GPIO1);
    Serial.println(n);
    Blinker.vibrate(); //震动反馈
    if(n == HIGH)
    {
      flag = 1;
    }
    else
    {
      flag = 0;
    }
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

void miotPowerState1(const String & state)
{
    BLINKER_LOG("need set OTA state: ",state);

    if (state == BLINKER_CMD_OFF) {//如果语音接收到是关闭烧录就设置GPIO口为高电平
        digitalWrite(GPIO1, HIGH);
        BlinkerMIOT.powerState("off");
        BlinkerMIOT.print();
    }
    else if (state == BLINKER_CMD_ON) {
        digitalWrite(GPIO1, LOW);
        BlinkerMIOT.powerState("on");
        BlinkerMIOT.print();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    pinMode(GPIO,OUTPUT);
    pinMode(GPIO1,OUTPUT);
    digitalWrite(GPIO,HIGH);//初始化，由于继电器是低电平触发。所以刚开始设为高电平
    digitalWrite(GPIO1,HIGH);//初始化，由于继电器是低电平触发。所以刚开始设为高电平
    WIFI_Init();//调用WIFI函数
    Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());//运行blinker
    Button1.attach(button1_callback);
    Button2.attach(button2_callback);
    BlinkerMIOT.attachPowerState(miotPowerState);//这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
    BlinkerMIOT.attachPowerState(miotPowerState1);//这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态


}

void loop()
{
    Blinker.run();
    //---------------------这一段用于使用过程中断网后从新联网用
    WiFi.status()!=WL_CONNECTED; 
    if(WIFI_Status = false)//WIFI连接失败
    {
        WIFI_Status = false;
        Serial.println("WiFi fail,The light flashes every 0.5 seconds."); 
        /*-----------这段用板载小灯的闪速显示未连接
        digitalWrite(ledPin, LOW);
        Blinker.delay(1000);           
        digitalWrite(ledPin, HIGH);
        Blinker.delay(1000);  
        */
     }
    else//使用flash中的信息去连接wifi失败，执行
    {
        Serial.println("Connected,The light flashes every 3 seconds.");  
        Serial.print("IP:");
        Serial.println(WiFi.localIP()); //串口打印连接成功的IP地址
        /*-----------这段用板载小灯的闪速显示已连接
        digitalWrite(ledPin, LOW);
        Blinker.delay(1000);           
        digitalWrite(ledPin, HIGH);
        Blinker.delay(3000);
        */
        }
    //---------------------这一段用于使用过程中断网后从新联网用
    if(flag == 0)
    {
      Serial.println("We Won't Burn.");
      Blinker.delay(500); 
    }
    else
    {
      Serial.println("We Will Burn.");
      Blinker.delay(500); 
    }
}
