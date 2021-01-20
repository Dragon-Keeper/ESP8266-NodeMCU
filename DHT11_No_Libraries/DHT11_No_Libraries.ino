const int dht11Pin = 12;   //连接12号引脚
//定义湿度传感器的几种状态和版本
#define DHTLIB_OK        0
#define DHTLIB_ERROR_CHECKSUM -1
#define DHTLIB_ERROR_TIMEOUT  -2
#define DHT11LIB_VERSION "0.4.1"
int humidity;
int temperature;

//dht11Read函数中的变量
uint8_t bits[5];  //一次完整的数据传输为40bit，每8bit为一个数据，即：数组个数为5
uint8_t cnt = 7; //这个是用来给每一个数据的每一位输入值时计数用的。
uint8_t idx = 0; //这个是给5个数组计数用的。

double Fahrenheit(double celsius) //摄氏温度度转化为华氏温度
{
  return 1.8 * celsius + 32;
}   

double Kelvin(double celsius)     //摄氏温度转化为开氏温度
{
  return celsius + 273.15;
}    

// 露点（点在此温度时，空气饱和并产生露珠）
// 参考: http://wahiduddin.net/calc/density_algorithms.htm
double dewPoint(double celsius, double humidity)
{
  double A0 = 373.15 / (273.15 + celsius);
  double SUM = -7.90298 * (A0 - 1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1) ;
  SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1) ;
  SUM += log10(1013.246);
  double VP = pow(10, SUM - 3) * humidity;
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}

// 快速计算露点，速度是5倍dewPoint()
// 参考: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity / 100);
  double Td = (b * temp) / (a - temp);
  return Td;
}

//根据时序图定义函数，用来读取具体数值
int dht11Read(int pin){
  pinMode(dht11Pin, OUTPUT);//引脚设置为输出模式
  digitalWrite(dht11Pin, LOW);//由于空闲时为高电平状态，需拉低等待DHT11相应
  delay(18);  //需要拉低必须大于18毫秒
  digitalWrite(dht11Pin, HIGH);//再拉高
  delayMicroseconds(40);//需要等待20~40微秒后
  pinMode(dht11Pin, INPUT);//这时DHT11开始接收主机的开始信号，故需设置为输入模式

//确认或超时
  unsigned int loopCnt = 10000;
  while (digitalRead(dht11Pin) == LOW)//从时序图中可以看出，接受数据一开始首先要读取80微秒的低电平，这里是一个等待，要把这80微秒等过去，但是有时候也有可能是传感器出现了故障，他一直发低电平，如果你持续等待不就相当于死机了，所以在这里要设置一个超时，也就是说要等待，但时间长了，就认为出问题了，返回一个异常信息。
    if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;
  loopCnt = 10000;
  while (digitalRead(dht11Pin) == HIGH)//从时序图中可以看出，在80微秒的低电平之后是80微秒的高电平，这里仍然要等待，超时的原理与上面的低电平一样。
    if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;
//从下面开始读取数据，因为40位，所以要循环40次，每次一位
  for (int i = 0; i < 40; i++)
  {
    loopCnt = 10000;
    while (digitalRead(dht11Pin) == LOW)//这句是等待低电平过去.根据时序图，可以看出，对于每一个bite位数据，都是由一个低电平和一个高电平组成，区分这一位数据是1还是0取决于高电平的时常，如果高电平的时常为70微秒则表示1，如果高电平的时常为26~28微秒则表示0，因此读取每一位数据时，都是先等待把50微秒的低电平等过去，然后判断高电平的时常，根据这个时常来判断这bite的数据是1还是0.
      if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;  //同样有可能超时失效。
    unsigned long t = micros(); //micros()记录板子上电的时间（微秒）
    loopCnt = 10000;
    while (digitalRead(dht11Pin) == HIGH) //这里就把高电平读出来了
      if (loopCnt-- == 0) return DHTLIB_ERROR_TIMEOUT;
    if ((micros() - t) > 40) bits[idx] |= (1 << cnt);//然后再次使用micros()函数获取当前时间，减去读取高电平之前的时间点，也就是这个高电平的时常了，然后看这个时常是否大于40微秒，如果大于就认为是1，否则就认为这位是0.那么这里又是怎么运算的呢？分析下：bits[idx]表示一个8位的数组，假设他是0000 0000，运算符“|=”表示按位进行或运算，然后再把运算的结果赋给运算符左边的变量。而(1 << cnt)表示把数字1的二进制表示法向左移动cnt位，移动后的空位用0来填充。因此，对于一个八位的1可以表示为：0000 0001，刚才的初始化过程中我们知道cnt的值为7，所以，把这个0000 0001左移七位就变成了：1000 0000.然后将这个数与0000 0000进行|=运算，之后bits[idx]中的值就是1000 0000。可见这段代码实现的功能就是如果的到的这位数据是1，就将他存储到bits[idx]相应的位上去。
    if (cnt == 0)   //cnt为0表示一个8位的数组已经装满了，要换到下一个八位的数组上去，于是就把cnt复原为7，idx++让idx直到bits的下一个八位的数组上去。
    {
      cnt = 7;    // restart at MSB
      idx++;      // next byte!
    }
    else cnt--;
    //如果cnt不为0就表示这个八位的数据还没有读完，这时只需要让cnt-1，来填充下一位数据就可以了。
    //注意，在初始化的过程中我们把这40位的数据都初始化为0了，所以只有当有1出现时才需要进行改变。
  }
  humidity    = bits[0];  //湿度,第1个8位是湿度的整数部分
  temperature = bits[2];  //温度,第3个8位是温度的整数部分
  uint8_t sum = bits[0] + bits[2];//总和
  if (bits[4] != sum) return DHTLIB_ERROR_CHECKSUM;
  return DHTLIB_OK;
}

void setup() {
  Serial.begin(115200);
  Serial.println("DHT11 TEST PROGRAM ");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.println();

}

void loop() {
  Serial.println("\n");
  int chk = dht11Read(dht11Pin);
  Serial.print("Read sensor: ");
  switch (chk)
  {
    case DHTLIB_OK:
      Serial.println("OK");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    default:
      Serial.println("Unknown error");
      break;
  }

  Serial.print("Humidity (%): ");
  Serial.println((float)humidity, 2);

  Serial.print("Temperature (oC): ");
  Serial.println((float)temperature, 2);

  Serial.print("Temperature (oF): ");
  Serial.println(Fahrenheit(temperature), 2);

  Serial.print("Temperature (K): ");
  Serial.println(Kelvin(temperature), 2);

  Serial.print("Dew Point (oC): ");
  Serial.println(dewPoint(temperature, humidity));

  Serial.print("Dew PointFast (oC): ");
  Serial.println(dewPointFast(temperature, humidity));

  delay(2000);
}

