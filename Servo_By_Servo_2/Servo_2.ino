#include <Servo.h>
 
#define PIN_SERVO 5
Servo myservo;
 
void setup()
{
  myservo.attach(PIN_SERVO);
}
 
void loop()
{
  myservo.write(0);
  delay(1000);
  myservo.write(80);
  delay(1000);
  myservo.write(160);
  delay(1000);
  myservo.write(80);
  delay(1000);
  myservo.write(0);
  delay(1000);
}
