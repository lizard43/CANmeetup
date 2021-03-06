# CAN Bus meetup talk
CAN Bus demo files from 4 Feb 2016 meetup
* http://www.meetup.com/FW-Dev/events/228176322/

![Demo](/images/demo.png)

This repository consists of:
* Arduino files used to simulate CAN Bus messages using the MCP2515 controller.
* Arduino files used to read CAN Bus messages and send to serial port as a JSON message.
* NodeJS files for reading serial port input and pushing out to websocket
* NodeJS files for a simple webserver that serves html file with RPM gauge in D3.js

The CAN_BUS_Shield folder contains files from 
* https://github.com/Seeed-Studio/CAN_BUS_Shield
* Refer to the README file in the CAN_BUS_Shield folder for info and its license
* I'm not using the Shield but this code works great for the cheap MCP2515 controller that you can find on ebay for ~$2.
* Move/Copy the CAN_BUS_Shield folder to your "libraries" folder in your Arduino IDE installation

![MCP2515](/images/mcp2515.jpg)

The connection between the Arduino and the MCP2515 module is SPI

![MCP2515 Wiring](/images/MCP2515-wiring.png)

Slides from this meetup are available at Slide Share:
* http://www.slideshare.net/roadster43
