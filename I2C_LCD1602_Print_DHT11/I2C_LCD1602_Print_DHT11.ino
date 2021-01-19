// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Wire.h> 
#include <LiquidCrystal_I2C.h> //引用I2C库
#include <DHT.h>
#define DHTPIN 12 // Digital pin connected to the DHT sensor
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
 
void setup()
{
  lcd.init();                  // 初始化LCD
  lcd.backlight();             //设置LCD背景等亮
  Serial.begin(115200);
  dht.begin(); //DHT开始工作
  Serial.println("DHT11 test!");
}
 
void loop()
{
  Serial.println("/n");
   // 两次检测之间，要等几秒钟，这个传感器有点慢。
  delay(2000);
  // 读温度或湿度要用250毫秒
  float h = dht.readHumidity();//读湿度
  float t = dht.readTemperature();//读温度，默认为摄氏度
  // 检测温度传感器是否工作
  if (isnan(h) || isnan(t)) {
    Serial.println(("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print("Humidity: ");//湿度
  Serial.print(h);
  Serial.print("%  Temperature: ");//温度
  Serial.print(t);
  lcd.clear();
  lcd.setCursor(0,0); //设置显示指针,第一行第一个显示位
  lcd.print("Humidity:");
  lcd.setCursor(9,0);//设置显示指针,第一行第9个显示位开始显示
  lcd.print(h);
  lcd.setCursor(0,1);//设置显示指针,第二行第一个显示位
  lcd.print("Temperature:");
  lcd.setCursor(12,1); //设置显示指针,第二行第12个显示位开始显示
  lcd.print(t);
  //lcd.print("         by L.L.");
  delay(1000);  
}
