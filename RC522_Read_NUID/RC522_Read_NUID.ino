/* Typical pin layout used:
  -----------------------------------------------------------------------------------------
              MFRC522      Arduino    ESP8266     Arduino    Arduino          Arduino
              Reader/PCD   Uno/101  Wemos D1 mini  Nano v3    Leonardo/Micro   Pro Micro
  Signal      Pin          Pin           Pin       Pin        Pin              Pin
  -----------------------------------------------------------------------------------------
  RST/Reset   RST          9             D3         D9         RESET/ICSP-5     RST
  SPI SS      SDA(SS)      10            D8        D10        10               10
  SPI MOSI    MOSI         11 / ICSP-4   D7        D11        ICSP-4           16
  SPI MISO    MISO         12 / ICSP-1   D6        D12        ICSP-1           14
  SPI SCK     SCK          13 / ICSP-3   D5        D13        ICSP-3           15
  SPI IRQ     IRQ          7             D4
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

  Serial.print("Decimal UID:");
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

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
