
#define BLINKER_WIFI
#define BLINKER_MIOT_LIGHT
#define BLINKER_PRINT Serial

#include <Blinker.h>

char auth[] = "c32bfc5d3b48";
char ssid[] = "test";
char pswd[] = "TF06680668";

// 新建组件对象
BlinkerButton Button1("btn-abc");
void miotBright(const String & bright)
{
    int colorW;
    BLINKER_LOG("need set brightness: ", bright);

    colorW = bright.toInt();
    
    analogWrite(LED_BUILTIN,1023-colorW*10.23);//0-1023    
    
    BLINKER_LOG("now set brightness: ", colorW);

    BlinkerMIOT.brightness(colorW);
    BlinkerMIOT.print();
}
void miotPowerState(const String & state)
{
    BLINKER_LOG("need set power state: ", state);

    if (state == BLINKER_CMD_ON) {
        digitalWrite(LED_BUILTIN, LOW);

        BlinkerMIOT.powerState("on");
        BlinkerMIOT.print();
    }
    else if (state == BLINKER_CMD_OFF) {
        digitalWrite(LED_BUILTIN, HIGH);

        BlinkerMIOT.powerState("off");
        BlinkerMIOT.print();
    }
}
// 按下按键即会执行该函数
void button1_callback(const String & state) {
    BLINKER_LOG("get button state: ", state);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

// 如果未绑定的组件被触发，则会执行其中内容
void dataRead(const String & data)
{
    BLINKER_LOG("Blinker readString: ", data);
}

void setup() {
    // 初始化串口
    Serial.begin(115200);

    #if defined(BLINKER_PRINT)
        BLINKER_DEBUG.stream(BLINKER_PRINT);
    #endif

    // 初始化有LED的IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    // 初始化blinker
    Blinker.begin(auth, ssid, pswd);
    Blinker.attachData(dataRead);
    Button1.attach(button1_callback);
    BlinkerMIOT.attachPowerState(miotPowerState);
    BlinkerMIOT.attachBrightness(miotBright);
}

void loop() {
    Blinker.run();
}
