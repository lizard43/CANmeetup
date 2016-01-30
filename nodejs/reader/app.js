//
// Simplae app to read data from serial port 
// and send to clients connected via websocket
//

var http = require("http"),
    url = require("url"),
    path = require("path"),
    io = require('socket.io')(app),
    sp = require("serialport");

var serialPortName = "/dev/ttyUSB0";
var webSocketPort = 8081;
if(process.argv.indexOf("-sp") != -1) { //does our flag exist?
    serialPortName = process.argv[process.argv.indexOf("-c") + 1]; //grab the next item
}
if(process.argv.indexOf("-ws") != -1) { //does our flag exist?
    webSocketPort = process.argv[process.argv.indexOf("-c") + 1]; //grab the next item
}

var SerialPort = sp.SerialPort;
var port = {};
try {
  port = new SerialPort(serialPortName, {
    baudrate: 115200,
    parser: sp.parsers.readline("\n")
  });
} catch(e) {
  console.log('error opening port: ', e);
}

var webSocket;
port.open(function () {
  console.log('opening serial port ' + serialPortName);
  port.on('data', function(data) {
    console.log(data);

    if (webSocket) {
      webSocket.emit('data', data);
    }
  });
});
 
var app = http.createServer(function(request, response) { 
  var listener = io.listen(app);
  listener.sockets.on('connection', function(socket){
    webSocket = socket;
    console.log('Web Socket Open');
  });
}).listen(parseInt(webSocketPort, 10));

console.log("Web Socket server running at\n  => http://localhost:" + webSocketPort + "/\nCTRL + C to shutdown");