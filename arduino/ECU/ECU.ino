// Simulate ECU
#include <mcp_can.h>
#include <SPI.h>

// simulated RPM
int rpm = 0;
int lprpm = 0;

// DTC/CEL
bool cel = false;
bool lpcel = false;

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

  // Send RPM using Toyota ID 0x2C4
  if (lprpm != rpm) {
     sendRPM(0x2C4, rpm);
  }

  // OBD request?
  obdRespond(rpm);

  // set check engine light if over rpm
  if (lprpm != rpm) {  
     setCEL(rpm);
  }
  
  // clear CEL is joystick depressed or via OBD2 reader
  clearCEL(zJoy);

  lprpm = rpm;
  lpcel = cel;
  delay(50);
}

// send RPM bytes using passed ID
void sendRPM(int id, int rpm) {
  // Send RPM on the CAN Bus where RPM = (A*256)+B)/4
  unsigned char rpmstmp[8] = {0x02, 0x41, 0x0C, 
    (unsigned char)((rpm*4) >> 8), (unsigned char)((rpm*4) & 0x00FF), 
    0,0,0};
  CAN.sendMsgBuf(id,0, 8, rpmstmp);  
  Serial.print("rpm: ");
  Serial.println(rpm);   
}

// using joystick reading, simulate rpm value
int calcRPM(int yJoy, int zJoy) {
  
//  Serial.print("yJoy: ");
//  Serial.println(yJoy);   
//  Serial.print("zJoy: ");
//  Serial.println(zJoy);   
  
  if (yJoy > 800) {
    rpm = rpm + (100 + random(-10,10));    
  }
  else if (yJoy < 100) {
    rpm = rpm - (100 + random(-10,10));    
  }

  if (rpm > 100) {
    rpm += random(-20,20);
  }
  
  if (rpm > 10000) {
    rpm = 10000;
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
  if (rpm > 9000) {
    if (cel == false) {
      unsigned char checkstmp[8] = {255, (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255), 
        (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255)};
      CAN.sendMsgBuf(0x89,0, 8, checkstmp);
      cel = true;
      Serial.println("CEL: SET");
    }
  }
}

// clear CEL if button depressed
void clearCEL(int button) {
  if ((button == 0) || ((lpcel != cel) & !cel)) {
    unsigned char checkstmp[8] = {0, (unsigned char)random(0,255), (unsigned char)random(0,255), (unsigned char)random(0,255), 
      (unsigned char)random(0,255), (unsigned char)random(0,255),(unsigned char)random(0,255), (unsigned char)random(0,255)};
    CAN.sendMsgBuf(0x89,0, 8, checkstmp);
    cel = false;    
    Serial.println("CEL: CLEAR");
  }
}

// monitor OBD Diagnostic request
void obdRespond(int rpm) {
  // check if data coming
  if(CAN_MSGAVAIL == CAN.checkReceive()) {             
    unsigned char len = 0;
    unsigned char buf[8];
    // read data,  len: data length, buf: data buf
    CAN.readMsgBuf(&len, buf);

    INT32U canId = CAN.getCanId();
    int id = canId;
    
    char data[100];
    sprintf(data, "{\"ID\":\"%04X\",\"data\":[ %i, %i, %i, %i, %i, %i, %i, %i ]}",
      id, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    Serial.println(data);    
    
    if (canId == 0x07DF) {
      Serial.println("PID Query");
      int mode = buf[1];
      int PID = buf[2];
      pidQuery(mode, PID, rpm);
    }
  }
}

// Sends response to PID Query
void pidQuery(int mode, int PID, int rpm) {

  int responseMode = 0x40 + mode;

  if (mode == 1) {
    Serial.println("Mode 1");

    // PIDs 0-20 supported
    if (PID == 0) {
      Serial.println("PID 0 : PIDs 0-20 supported");      
      // 98 3B 80 11
      // supported PIDs encoded, which translates to PIDs: 01, 04, 05, 0B, 0C, 0D, 0F, 10, 11, 1C, 20
      unsigned char checkstmp[8] = {6,responseMode,PID, 
        (unsigned char)0x98, (unsigned char)0x3B, (unsigned char)0x80, (unsigned char)0x11, 
        0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // DTCs
    else if (PID == 1) {
      Serial.println("PID 1 : DTC");
      unsigned char checkstmp[8] = {6,responseMode,PID, 
         cel ? 0x81 : 0x0,0xB0,0,0,
         0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // load value
    else if (PID == 0x04) {
      Serial.println("PID 04 : Load Value");
      unsigned char checkstmp[8] = {3,responseMode,PID, 
        75 + random(-5,5), 
        0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // coolant temp
    else if (PID == 0x05) {
      Serial.println("PID 05 : Coolant Temp");
      unsigned char checkstmp[8] = {3,responseMode,PID, 
        100 + random(-10,10), 
        0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // Intake Manifold Pressure
    else if (PID == 0x0B) {
      Serial.println("PID 0B : Intake Manifold Pressure");
      unsigned char checkstmp[8] = {3,responseMode,PID, 
        100 + random(-10,10), 
        0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // RPM
    else if (PID == 0x0C) {
      Serial.println("PID 0C : RPM");
      unsigned char checkstmp[8] = {4,responseMode,PID, 
        (unsigned char)((rpm*4) >> 8), (unsigned char)((rpm*4) & 0x00FF), 
          0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // Speed
    else if (PID == 0x0D) {
      Serial.println("PID 0D : Speed");
      unsigned char checkstmp[8] = {3,responseMode,PID, 
        100 + random(-10,10), 
        0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // air intake temp
    else if (PID == 0x0F) {
      Serial.println("PID 05 : Air Intake Temp");
      unsigned char checkstmp[8] = {3,responseMode,PID, 
        50 + random(-2,2), 
        0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // MAF
    else if (PID == 0x10) {
      Serial.println("PID 10 : MAF");
      // ((A*256)+B) / 100
      unsigned char checkstmp[8] = {4,responseMode,PID, 
        150  + random(-10,10), 
        100 + random(-90,90),
        0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // Throttle position
    else if (PID == 0x11) {
      Serial.println("PID 11 : Throttle position");
      unsigned char checkstmp[8] = {3,responseMode,PID, 
        100 + random(-10,10), 
        0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // ODB Standard
    // 2 = OBD as defined by the EPA
    else if (PID == 0x1C) {
      Serial.println("PID 1C : OBS Standard");
      unsigned char checkstmp[8] = {6,responseMode,PID, 
         2,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }

    // PIDs supported [21 - 40]
    else if (PID == 0x20) {
      Serial.println("PID 20 : PIDs supported [21 - 40]");
      // 00 00 00 00
      // supported PIDs encoded, which translates to PIDs: NONE
      unsigned char checkstmp[8] = {6,responseMode,PID,
        0,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);    
    }

    // PIDs supported [41 - 60]
    else if (PID == 0x40) {
      Serial.println("PID 40 : PIDs supported [41 - 60]");
      // 00 00 00 00
      // supported PIDs encoded, which translates to PIDs: NONE
      unsigned char checkstmp[8] = {6,responseMode,PID,
        0,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);    
    }

    // PIDs supported [61 - 80]
    else if (PID == 0x60) {
      Serial.println("PID 60 : PIDs supported [61 - 80]");
      // 00 00 00 00
      // supported PIDs encoded, which translates to PIDs: NONE
      unsigned char checkstmp[8] = {6,responseMode,PID,
        0,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);    
    }

    // PIDs supported [81 - A0]
    else if (PID == 0x80) {
      Serial.println("PID 80 : PIDs supported [81 - A0]");
      // 00 00 00 00
      // supported PIDs encoded, which translates to PIDs: NONE
      unsigned char checkstmp[8] = {6,0xC0,0,0,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);    
    }

    // PIDs supported [A1 - C0]
    else if (PID == 0xA0) {
      Serial.println("PID A0 : PIDs supported [A1 - C0]");
      // 00 00 00 00
      // supported PIDs encoded, which translates to PIDs: NONE
      unsigned char checkstmp[8] = {6,responseMode,PID,
        0,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);    
    }

    // PIDs supported [C1 - E0]
    else if (PID == 0xC0) {
      Serial.println("PID C0 : PIDs supported [C1 - E0]");
      // 00 00 00 00
      // supported PIDs encoded, which translates to PIDs: NONE
      unsigned char checkstmp[8] = {6,responseMode,PID,
        0,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);    
    }

    // PIDs supported [E1 - 100]
    else if (PID == 0xE0) {
      Serial.println("PID E0 : PIDs supported [E1 - 100]");
      // 00 00 00 00
      // supported PIDs encoded, which translates to PIDs: NONE
      unsigned char checkstmp[8] = {6,responseMode,PID,
        0,0,0,0,0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);    
    }

  }

  else if (mode == 3) {
    Serial.println("Mode 3"); 

    if (PID == 0) {
      Serial.println("PID 0 : DTC");
      if (cel) {
        // P0133    
        unsigned char checkstmp[8] = {4,responseMode,PID, 
          (unsigned char)0x01, (unsigned char)0x33, 
          0,0,0};
        CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
      }
      else {
        unsigned char checkstmp[8] = {4,responseMode,PID, 
          (unsigned char)0x00, (unsigned char)0x00, 
          0,0,0};
        CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
      }
    }
  }

  else if (mode == 4) {
    Serial.println("Mode 4"); 

    if (PID == 0) {
      Serial.println("PID 0 : Clear DTC");
      cel = false;
    }
  }

  else if (mode == 7) {
    Serial.println("Mode 7"); 

    if (PID == 0) {
      Serial.println("PID 0");      
    }
  }

  else if (mode == 9) {
    Serial.println("Mode 9"); 

    // PIDs 01-20 supported
    if (PID == 0) {
      Serial.println("PID 0");      
      // FE 1F A8 13
      // supported PIDs encoded, which translates to PIDs:  01, 02, 03, 04, 05, 06, 07, 0C, 0D, 0E, 0F, 10, 11, 13, 15, 1C, 1F and 20
      unsigned char checkstmp[8] = {6,responseMode,PID, 
        (unsigned char)0xC0, (unsigned char)0, (unsigned char)0x0, (unsigned char)0x0, 
        0};
      CAN.sendMsgBuf(0x7E8,0, 8, checkstmp);
    }
    
    if (PID == 2) {
      Serial.println("PID 2");      
      unsigned char checkstmp[17] = {17,responseMode,PID, 
        1,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0};
      CAN.sendMsgBuf(0x7E8,0, 17, checkstmp);
    }    
  }
  
  else if (mode == 10) {
    Serial.println("Mode 10"); 

    if (PID == 0) {
      Serial.println("PID 0");      
    }
  }

}
