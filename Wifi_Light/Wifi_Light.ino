#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <ESP8266WiFi.h>
#include <Blinker.h>

char auth[] = "8520021e8f7c";  //设备key，打开点灯app，添加设备，选择arduino设备，选择wifi接入，这里就会获得一个密钥，把密钥填写在这里。
char ssid[] = "DarthVader";  //路由器wifi ssid
char pswd[] = "aa123456";  //路由器wifi 密码

int GPIO=0;//定义GPIO口用于控制继电器
int ledPin = 2; // GPIO2 of ESP8266-01S

#define BUTTON_1 "ButtonKey"


BlinkerButton Button1("Light_key1");//这里需要根据自己在BLINKER里面设置的名字进行更改

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
    Blinker.begin(auth, ssid, pswd);
    Button1.attach(button1_callback);
    BlinkerMIOT.attachPowerState(miotPowerState);//这段代码一定要加，不加小爱同学控制不了,务必在回调函数中反馈该控制状态


}

void loop()
{
    Blinker.run();
    digitalWrite(ledPin, HIGH);
    Blinker.delay(1000);           
    digitalWrite(ledPin, LOW);
    Blinker.delay(1000);  
}
