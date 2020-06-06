"ESP8266_PWMHeater.ino
Automatic Servoed heater uses PID to manage a set temperature using dewpoint and termperature readings. " 
This application runs on the ESP8266-01 wifi-enabled SoC device to make local temperature sensor readings and use them to servo control a heating element. The target temperature is provided by a subscription to an environment dewpoint sensor hosted on the nearby MQTT server.  This also provides a restful interface for direct query using curl or http of the servo state and target temperature.</br>
In my arrangement, Node-red flows are used to listen for and graph the updated readings in the dashboard UI. </br>
The unit is setup for 3 pin io operation ( SDL, SCL, PWM out) and is expecting to see SDA on GPIO0, SCL on GPIO2 and PWMout on GPIO3(Rx). 
. 

Dependencies:
Arduino 1.8+, </br>
ESP8266 V2.4+ </br>
Arduino MQTT client (https://pubsubclient.knolleary.net/api.html)</br>
Arduino JSON library (pre v6) </br>
Arduino PID servo library </br>

Testing
Access by serial port  - Tx only is available from device at 115,600 baud at 3.3v. THis provides debug output .</br>
Wifi is used for MQTT subscription and reporting only on: /skybadger/sensors/<sensor type>/<host> </br>
Update for subscribing to your own sensor network.</br>
ESP8266HttpServer is used to service  web requests </br>
ESP8266HTTPUpdateServer is used to update the binary via OTA upload. </br>
Use http://espPWM01/temperature to receive json-formatted output of current temperature and setpoints reading. </br>

Use:
I use mine to source a dashboard via hosting the mqtt server in node-red. It runs off a solar-panel supply in my observatory dome. </br>

ToDo:
Expand to drive multiple PWM pins on a larger ESP format chip. </br>
