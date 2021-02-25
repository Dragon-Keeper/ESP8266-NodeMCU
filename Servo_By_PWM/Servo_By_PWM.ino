#include <Arduino.h>
int servoPin = 4;

void setup()  //初始化内容
{
  Serial.begin(115200);
  pinMode(servoPin, OUTPUT); //设定舵机接口为输出接口
}

void loop()   //主循环
{
  for ( int angle = 0; angle < 180; angle += 10)
  {
    servo(angle); //调用函数传值直接动
  }

}

void servo(int angle) { //定义一个脉冲函数
  //发送50个脉冲
  for (int i = 0; i < 50; i++) {
    int pulsewidth = (angle * 11) + 500; //将角度转化为500-2480的脉宽值
    digitalWrite(servoPin, HIGH);   //将舵机接口电平至高
    delayMicroseconds(pulsewidth);  //延时脉宽值的微秒数
    digitalWrite(servoPin, LOW);    //将舵机接口电平至低
    delayMicroseconds(20000 - pulsewidth);
  }
  delay(100);
}
