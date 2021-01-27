/* Typical pin layout used:
  -----------------------------------------------------------------------------------------
              MFRC522      Arduino    ESP8266     Arduino    Arduino          Arduino
              Reader/PCD   Uno/101  Wemos D1 mini  Nano v3    Leonardo/Micro   Pro Micro
  Signal      Pin          Pin           Pin       Pin        Pin              Pin
  -----------------------------------------------------------------------------------------
  RST/Reset   RST          9             D3 / 0    D9         RESET/ICSP-5     RST
  SPI SS      SDA(SS)      10            D8 / 15   D10        10               10
  SPI MOSI    MOSI         11 / ICSP-4   D7 / 13   D11        ICSP-4           16
  SPI MISO    MISO         12 / ICSP-1   D6 / 12   D12        ICSP-1           14
  SPI SCK     SCK          13 / ICSP-3   D5 / 14   D13        ICSP-3           15
  SPI IRQ     IRQ          7             D4 / 2
  1. 时钟(SPI CLK, SCLK) 
  2. 片选(CS) 
  3. 主机输出、从机输入(MOSI) 
  4. 主机输入、从机输出(MISO) 产生时钟信号的器件称为主机。
  5. IRQ中断才用到的，没有用到可以不接。
  -----------------------------------------------------------------------------------------

              LCD1602     NodeMCU 8266 LoLin V3
  Signal      Pin          Pin
  -------------------------------------
  SPI SS      SDA         D2
  SPI SCL     SCL         D1
  -----------------------------------------------------------------------------------------

             DHT11     NodeMCU 8266 LoLin V3
  Signal      Pin          Pin
  -------------------------------------
  SPI SS      SDA         D9 （原来接D6）
  -----------------------------------------------------------------------------------------
  小卡
  Hexadecimal UID:96CAC893
  Decimal UID:150202200147
  白卡
  Hexadecimal UID:1D6C345B
  Decimal UID:291085291
*/
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 15
#define RST_PIN 0

MFRC522 rfid(SS_PIN, RST_PIN); //实例化类

// 初始化数组用于存储读取到的NUID
byte nuidPICC[4];

void setup() {
  Serial.begin(115200);
  SPI.begin(); // 初始化SPI总线
  rfid.PCD_Init(); // 初始化 MFRC522
}

void loop() {

  // 找卡
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // 验证NUID是否可读
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  // 检查是否MIFARE卡类型
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println("Not Supported The Type Of This Card");
    return;
  }

  // 将NUID保存到nuidPICC数组
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  Serial.println(".");
  Serial.print("Hexadecimal UID:");
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

 if(nuidPICC[0]==0x96 && nuidPICC[1]==0xCA && nuidPICC[2]==0xC8 && nuidPICC[3]==0x93) 
    { //该卡号可以更改为自己的IC卡号，包括校园卡等
      Serial.println("Hello Arduino!");
      //这里写入如果号码对的话如何操作
      delay(2000);
    }
    else
    { //若卡号与上述不符
      Serial.println("Hello unkown guy!");
      //这里写入如果号码不对的话如何操作
      delay(2000);
    }
  //Serial.print("Decimal UID:"); 读取十进制的Nuid号码
  //printDec(rfid.uid.uidByte, rfid.uid.size);
  //Serial.println();

  // 使放置在读卡区的IC卡进入休眠状态，不再重复读卡
  rfid.PICC_HaltA();

  // 停止读卡模块编码
  rfid.PCD_StopCrypto1();
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : "");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : "");
    Serial.print(buffer[i], DEC);
  }
}

