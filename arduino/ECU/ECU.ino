// Simulate ECU
#include <mcp_can.h>
#include <SPI.h>

const int SPI_CS_PIN = 10;

int ledPin = 13;   // select the pin for the LED
const int ledHIGH    = 1;
const int ledLOW     = 0;

// Joystick
const int xPin = A0;
const int yPin = A1;
const int btPin = 7;
 
// simulated RPM
int rpm = 0;

MCP_CAN CAN(SPI_CS_PIN);   

void setup()
{
  pinMode(btPin,INPUT);
  digitalWrite(btPin, HIGH);
  
  pinMode(ledPin, OUTPUT);  // declare the ledPin as an OUTPUT
  
  Serial.begin(115200);
  Serial.println("Setup");
  
START_INIT:

  // init can bus : baudrate = 500k
  if(CAN_OK == CAN.begin(CAN_500KBPS))                   
  {
    Serial.println("ECU CAN BUS init ok!");
  }
  else
  {
    Serial.println("ECU CAN BUS init fail");
    Serial.println("Attempting Init ECU CAN BUS again");
    delay(100);
    goto START_INIT;
  }
}

void loop()
{   
  // Read Joystick
  int yJoy = analogRead(yPin);
  int zJoy = digitalRead(btPin);
    
  // calc simulated RPM
  if (yJoy > 800) {
    rpm = rpm + 10;    
  }
  else if (yJoy > 530){
    rpm = rpm + 5;    
  }
  else if (yJoy < 100) {
    rpm = rpm - 10;    
  }
  else if (yJoy < 500) {
    rpm = rpm - 5;    
  }
  
  if (rpm > 255) {
    rpm = 255;
  }
  if (rpm < 1) {
    rpm = 0;
  }

  // reset if button depressed
  if (zJoy == 0) {
    rpm = 0;
  }

  // Send RPM on the CAN Bus
  Serial.print("rpm: ");
  Serial.println(rpm);
  // RPM = (A*256)+B)/4  
  unsigned char rpmstmp[8] = {0x03, 0x41, 0x0C, ((rpm*4) >> 8), ((rpm*4) & 0x00FF), 
    0,0,0,0};
  CAN.sendMsgBuf(0x7E8,0, 8, rpmstmp);
  delay(50);

  // set check engine light if over rpm
  if (rpm > 230) {
    unsigned char checkstmp[8] = {255, random(0,255), random(0,255), random(0,255), 
      random(0,255), random(0,255),random(0,255), random(0,255)};
    CAN.sendMsgBuf(0x89,0, 8, checkstmp);
    Serial.print("CEL: ");
    Serial.println(255);    
    delay(50);
  }
  // clear check engine light if engine shutdown
  else if (rpm == 0) {
    unsigned char checkstmp[8] = {0, random(0,255), random(0,255), random(0,255), 
      random(0,255), random(0,255),random(0,255), random(0,255)};
    CAN.sendMsgBuf(0x89,0, 8, checkstmp);
    Serial.print("CEL: ");
    Serial.println(0);    
    delay(50);
  }
}
