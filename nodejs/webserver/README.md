# CAN Bus meetup talk

* NodeJS files for a simple webserver that serves html file with RPM gauge in D3.js

This webserver serves the html and javascript files in this folder. 
The webserver serves files at 8080.
The javascript attempts to connect to a webserver on localhost at default port of 8081

Because this is a demo and I'm serving local files, I've taken liberty of relaxing security by setting the http response headers of
* 'Access-Control-Allow-Origin': '*',
* 'Access-Control-Allow-Methods': 'GET',
* 'Access-Control-Allow-Headers': 'Content-Type'

The Serial Port reader app should be started before this app so that the web socket server is available.

Install the nodeJS depencies like this:
* npm install

Run like this:
* node webserver

Then browse to:
* http://localhost:8080/demo.html
* Please use a modern html5 browser, preferably Chrome, not IE