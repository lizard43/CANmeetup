// receive CAN message and prints to serial port as JSON
#include <SPI.h>
#include "mcp_can.h"

const int SPI_CS_PIN = 10;
const int LED=13;

MCP_CAN CAN(SPI_CS_PIN);                                    

void setup()
{
    Serial.begin(115200);
    pinMode(LED,OUTPUT);

START_INIT:

    if(CAN_OK == CAN.begin(CAN_500KBPS)) 
    {
        Serial.println("Receiver CAN BUS init ok!");
    }
    else
    {
        Serial.println("Receiver CAN BUS init fail");
        Serial.println("Receiver Init CAN BUS again");
        delay(100);
        goto START_INIT;
    }
}

void loop()
{
    unsigned char len = 0;
    unsigned char buf[8];

    // check if data coming
    if(CAN_MSGAVAIL == CAN.checkReceive()) {           

        // read data,  len: data length, buf: data buf
        CAN.readMsgBuf(&len, buf);

        INT32U canId = CAN.getCanId();
        int id = canId;

        char data[100];
        sprintf(data, "{\"ID\":\"%04X\",\"data\":[ %i, %i, %i, %i, %i, %i, %i, %i ]}",
          id, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        Serial.println(data);

        if (canId == 0x07DF) {
          Serial.println("ECU Response");  
//          7E9 03 7F 23 80
//        07E8 06 41 00 98 3B 80 11 00
          unsigned char stmp[8] = {06, 41, 00, 98, 0x3B, 80, 11, 00};
          CAN.sendMsgBuf(0x07E8,0, 8, stmp);
        }
    }
}
