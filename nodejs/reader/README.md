# CAN Bus meetup talk

NodeJS files for reading serial port input and pushing out to websocket
* Install
  * npm install
* Run
  * sudo node app.js
    * this uses /dev/ttyUSB0
    * this uses 8081 for websocket port
    * sudo is needed to open serial port
  * OR
    * sudo node app.js -sp serialport -ws websocket
      * this uses whatever serial port name you pass in
      * this uses whatever websocket port name you pass in
      * sudo is needed to open serial port
