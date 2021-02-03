// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
 /* Typical pin layout used:
 *             LCD1602     NodeMCU 8266 LoLin V3
 * Signal      Pin          Pin           
 * -------------------------------------         
 * SPI SS      SDA         D2           
 * SPI SCL     SCL         D1
 * -----------------------------------------------------------------------------------------
 *            DHT11     NodeMCU 8266 LoLin V3
 * Signal      Pin          Pin           
 * -------------------------------------         
 * SPI SS      SDA         D6   
 */
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> //引用I2C库
#include <DHT.h>
#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <ESP8266WiFi.h>
#include <Blinker.h>

int count = 0;
bool WIFI_Status = true;
char auth[] = "8520021e8f7c";  //你的设备key
int GPIO = 0;//定义GPIO口用于控制继电器
int ledPin = 2; //通过闪烁的快慢提示是否连上WiFi（未连上：快速闪烁；已连上：3秒一次）
#define BUTTON_1 "ButtonKey"

//新建数据类型组件对象，作用：将数据传输到手机blinker app
BlinkerNumber HUMI("humi");    //定义湿度数据键名
BlinkerNumber TEMP("temp");    //定义温度数据键名
float humi_read = 0, temp_read = 0;//定义浮点型全局变量 储存传感器读取的温湿度数据

#define DHTPIN 12 // 接NodeMCU D6 获取温湿度数据 Digital pin connected to the DHT sensor
//定义类型，DHT11或者其它
#define DHTTYPE DHT11
//进行初始设置 
DHT dht(DHTPIN, DHTTYPE);
//设置LCD1602设备地址，这里的地址是0x3F，一般是0x20，或者0x27，具体看模块手册
LiquidCrystal_I2C lcd(0x3F,16,2); 

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

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

void heartbeat()
{
    HUMI.print(humi_read);        //给blinkerapp回传湿度数据
    TEMP.print(temp_read);        //给blinkerapp回传温度数据
}

void miotQuery(int32_t queryCode)      //小爱同学语音命令反馈
{
    BLINKER_LOG("MIOT Query codes: ", queryCode);

            int humi_read_int=humi_read;     //去掉湿度浮点数
            BlinkerMIOT.humi(humi_read_int); //小爱反馈湿度属性
            BlinkerMIOT.temp(temp_read);     //小爱反馈温度属性
            BlinkerMIOT.print();//将以上属性发送给小爱，使得小爱可以接收到温湿度的数据

}

void setup()
{
  lcd.init();                  // 初始化LCD
  lcd.backlight();             //设置LCD背景等亮
  Serial.begin(115200);
  dht.begin(); //DHT开始工作
  Serial.println("DHT11 test!");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(GPIO,OUTPUT);
  digitalWrite(GPIO,HIGH);//初始化，由于继电器是低电平触发。所以刚开始设为高电平
  // Blinker.begin(auth, ssid, pswd);
  WIFI_Init();//调用WIFI函数
  Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());//运行blinker
  Button1.attach(button1_callback);
  BlinkerMIOT.attachPowerState(miotPowerState);//这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态
  
  BLINKER_DEBUG.stream(Serial);
  BLINKER_DEBUG.debugAll();
  Blinker.attachHeartbeat(heartbeat);//将传感器获取的数据传给blinker app上
  dht.begin();//初始化DHT传感器
  //在回调函数中反馈该控制状态
  BlinkerMIOT.attachQuery(miotQuery);//每次呼出小爱同学，就会调用miotQuery()函数
}
 
void loop()
{
  Serial.println("/n");
   // 两次检测之间，要等几秒钟，这个传感器有点慢。
  Blinker.delay(500);
  // 读温度或湿度要用250毫秒
  float h = dht.readHumidity();//读取DHT11传感器的湿度    并赋值给h
  float t = dht.readTemperature();//读取传感器的摄氏温度  并赋值给t
  // 检测温度传感器是否工作
  if (isnan(h) || isnan(t)) { //判断是否成功读取到温湿度数据
    Serial.println(("Failed to read from DHT sensor!"));
    return;
  }
  BLINKER_LOG("Humidity: ", h, " %");
  BLINKER_LOG("Temperature: ", t, " *C");  
  humi_read = h;//将读取到的湿度赋值给全局变量humi_read
  temp_read = t;//将读取到的温度赋值给全局变量temp_read
  Serial.print("Humidity: ");//湿度
  Serial.print(h);
  Serial.print("%  Temperature: ");//温度
  Serial.print(t);
  lcd.clear();
  lcd.setCursor(0,0); //设置显示指针,第一行第一个显示位
  lcd.print("T&H:");
  lcd.setCursor(4,0);//设置显示指针,第一行第4个显示位开始显示
  lcd.print(t,0);
  lcd.setCursor(7,0);//设置显示指针,第一行第9个显示位开始显示
  lcd.print("&");
  lcd.setCursor(9,0);//设置显示指针,第一行第9个显示位开始显示
  lcd.print(h,0); //后面的数字表示显示小数点的位数，0表示不显示
  lcd.setCursor(11,0);//设置显示指针,第一行第12个显示位开始显示
  lcd.print("%");
  Blinker.delay(500);  
  Blinker.run();
  WiFi.status()!=WL_CONNECTED;
  if(WIFI_Status = false)//WIFI连接失败
  {
      WIFI_Status = false;
      Serial.println("WiFi fail,The light flashes every 0.5 seconds."); 
      digitalWrite(ledPin, LOW);
      Blinker.delay(500);           
      digitalWrite(ledPin, HIGH);
      Blinker.delay(500);  
   }
  else//使用flash中的信息去连接wifi失败，执行
  {
      Serial.println("Connected,The light flashes every 3 seconds.");  
      Serial.print("IP:");
      Serial.println(WiFi.localIP()); //串口打印连接成功的IP地址
      digitalWrite(ledPin, LOW);
      Blinker.delay(1000);           
      digitalWrite(ledPin, HIGH);
      Blinker.delay(3000);  
      }
}
