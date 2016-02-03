// Simulate ECU
#include <mcp_can.h>
#include <SPI.h>

// simulated RPM
int rpm = 0;

const int SPI_CS_PIN = 10;

int ledPin = 13;   // select the pin for the LED
const int ledHIGH    = 1;
const int ledLOW     = 0;

// Joystick
const int xPin = A0;
const int yPin = A1;
const int btPin = 7;
 
MCP_CAN CAN(SPI_CS_PIN);   

void setup() {
  pinMode(btPin,INPUT);
  digitalWrite(btPin, HIGH);
  
  pinMode(ledPin, OUTPUT);  // declare the ledPin as an OUTPUT
  
  Serial.begin(115200);
  Serial.println("Setup");
  
START_INIT:

  // init can bus : baudrate = 500k
  if(CAN_OK == CAN.begin(CAN_500KBPS)) {
    Serial.println("ECU CAN BUS init ok!");
  }
  else {
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
  rpm = calcRPM(yJoy, zJoy);

  // Send RPM using ID 0x7E8  
  sendRPM(0x7E8, rpm);

  // OBD request for RPM?
  obdRespond(rpm);

  // set check engine light if over rpm
  setCEL(rpm);
  
  // clear CEL is joystick depressed
  clearCEL(zJoy);

  delay(200);
}

// send RPM bytes using passed ID
void sendRPM(int id, int rpm) {
  // Send RPM on the CAN Bus where RPM = (A*256)+B)/4
  unsigned char rpmstmp[8] = {0x03, 0x41, 0x0C, 
    (unsigned char)((rpm*4) >> 8), (unsigned char)((rpm*4) & 0x00FF), 
    0,0,0,};
  CAN.sendMsgBuf(id,0, 8, rpmstmp);  
  Serial.print("rpm: ");
  Serial.println(rpm);   
}

// using joystick reading, simulate rpm value
int calcRPM(int yJoy, int zJoy) {
  Serial.print("yJoy: ");
  Serial.println(yJoy);   
  Serial.print("zJoy: ");
  Serial.println(zJoy);   
  
  if (yJoy > 800) {
    rpm = rpm + 5;    
  }
  else if (yJoy < 100) {
    rpm = rpm - 5;    
  }
  
  if (rpm > 255) {
    rpm = 255;
  }
  if (rpm < 1) {
    rpm = 0;
  }

  // reset rpm if button depressed
  if (zJoy == 0) {
    rpm = 0;
  }

  return rpm;
}

// if RPM is over max, set Check Engine Light
void setCEL(int rpm) {
  if (rpm > 230) {
    unsigned char checkstmp[8] = {255, (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255), 
      (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255)};
    CAN.sendMsgBuf(0x89,0, 8, checkstmp);
    Serial.print("CEL: SET");
  }
}

// clear CEL if button depressed
void clearCEL(int button) {
  if (button == 0) {
    unsigned char checkstmp[8] = {0, (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255), 
      (unsigned char)random(0,255), (unsigned char)random(0,255),(unsigned char)random(0,255), (unsigned char)random(0,255)};
    CAN.sendMsgBuf(0x89,0, 8, checkstmp);
    Serial.print("CEL: CLEAR");
  }
}

// monitor OBD Diagnostic RPM request
void obdRespond(int rpm) {
  // check if data coming
  if(CAN_MSGAVAIL == CAN.checkReceive()) {             
    unsigned char len = 0;
    unsigned char buf[8];
    // read data,  len: data length, buf: data buf
    CAN.readMsgBuf(&len, buf);
    
    INT32U canId = CAN.getCanId();
    int id = canId;
    
    if (canId == 0x07DF) {
      Serial.println("ECU Response");
      sendRPM(0x7E8, rpm);
    }
  }
}

