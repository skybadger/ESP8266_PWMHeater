/*
 Program to manage attached PWM heaters in heater bands used on telescopes and eyepieces to keep dewing away.
 Each heater is reqquired to have a local i2c temp sensor 
 Each heater may optonally be controlled by a manual encoder-based switch
 When an encoder is presrent, there will be a channel select button to manage too. 
 The choice of control is either ADC for a potetiometer or encoder for rotatinal position like on a radio.
 Supports REST web interface on port 80 
 REST interface supports control, status and intial setup for user IP environment. 
 DHCP network configuration is assumed.   

 To do:
 Add manual support (much later) 
 Web control is preferred. 
 
 Update
 Select and Configure temp sensor on i2c 

 Done: 

 Layout:
 GPIO 4,2 to SDA
 GPIO 5,0 to SCL 
 GPIO3 to PWM transistor gate
 All 3.3v logic. 

*/

#include "ProjectDefs.h"
#include "DebugSerial.h"
#include <esp8266_peri.h> //register map and access
#include <ESP8266WiFi.h>
#include <PubSubClient.h> //https://pubsubclient.knolleary.net/api.html
#include <Wire.h>         //https://playground.arduino.cc/Main/WireLibraryDetailedReference
#include <Time.h>         //Look at https://github.com/PaulStoffregen/Time for a more useful internal timebase library
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoJson.h>  //https://arduinojson.org/v5/api/
#include <Encoder.h>      //http://www.pjrc.com/teensy/td_libs_Encoder.html
#include <Math.h>         //Used for PI.
#include <PID_v1.h>       //PID library @
#include <GDBStub.h> //Debugging stub for GDB
#include "SkybadgerStrings.h"

//Ntp dependencies - available from ESP v2.4
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>
time_t now; //use as 'gmtime(&now);'

char* defaultHostname        = "espPWM00";
char* thisID                 = "espPWM00";
char* myHostname             = "espPWM00";

WiFiClient espClient;
PubSubClient client(espClient);
volatile bool callbackFlag = 0;
//Used by MQTT reconnect
volatile bool timerSet  = false;

// Create an instance of the web server
// specify the port to listen on as an argument
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

//Hardware device system functions - reset/restart etc
EspClass device;
ETSTimer timer, timeoutTimer;
volatile bool newDataFlag = false;
volatile bool timeoutFlag = false;

//Buffer space for averaging etc. 
const int MAXDATA = 3600;

//Function definitions
void onTimer(void);
void onTimeoutTimer(void);
void callback(char* topic, byte* payload, unsigned int length) ;
void publishHealth(void);
void publishHeater( void );
uint32_t inline ICACHE_RAM_ATTR myGetCycleCount();

//Temperature sensor - TODO 
#include "i2c.h"          // Temp/pressure sensor.
#include "i2c_Sensor.h"
#include "i2c_BMP280.h"
bool tempSensPresent = false;
BMP280 bmp280;
double currentTemperature; //Needed as double for servo.
int32_t iCurrentTemperature = 0;
//PWM setup
#define N_PWM 1
#define PIN_OUTPUT  0x05

float delta = 2.5F; //Target temperature differnce to seek above dewpoint

//Externally sourced dewpoint temperature
float dewpoint = 5.0F;

double  targetTemperature = dewpoint + delta;
bool    dewpointTrackState = false;
double  output = 0;
// External pins used
int pwm_pin[N_PWM] = {3,};

//Define the aggressive and conservative Tuning Parameters
double aggKp=4, aggKi=0.2, aggKd=1;
double consKp=1, consKi=0.05, consKd=0.25;
//Specify the links and initial tuning parameters
PID aPid( &currentTemperature, &output, &targetTemperature, consKp, consKi, consKd, MANUAL );
boolean masterPidEnable = false, aPidEnable = false; 

//EEprom handlers have a dependency from handlers hence come first. 
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include "PWMHeaterEeprom.h"
#include "Skybadger_common_funcs.h"
//Web Handler function definitions
#include "PWMH_Handlers.h"
#include "PWMHSetupHandlers.h"

void setup_wifi()
{
  int i=0;
  WiFi.hostname( myHostname );
  WiFi.mode(WIFI_STA);
  Serial.println(ssid1);
  
  WiFi.begin(ssid1, password1);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
      Serial.print(".");
    if ( i++ > 400 )
      device.restart();      
  }
  Serial.println("WiFi connected");
  Serial.printf("SSID connected: %s, WiFi strength: %i dBm \n\r", WiFi.SSID().c_str(), WiFi.RSSI() );
  Serial.printf("Hostname: %s\n\r",      WiFi.hostname().c_str() );
  Serial.printf("IP address: %s\n\r",    WiFi.localIP().toString().c_str() );
  Serial.printf("DNS address 0: %s\n\r", WiFi.dnsIP(0).toString().c_str() );
  Serial.printf("DNS address 1: %s\n\r", WiFi.dnsIP(1).toString().c_str() );

  delay(5000);

  //Setup sleep parameters
  //WiFi.mode(WIFI_NONE_SLEEP);
  wifi_set_sleep_type(NONE_SLEEP_T);
}

void setup()
{
  Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.println(F("ESP starting."));
  gdbstub_init();
  
  //Start NTP client
  struct timezone tzone;
  tzone.tz_minuteswest = 480;
  tzone.tz_dsttime = DST_MN;
  //configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
  //configTime( 0, timeServer1, timeServer1, timeServer3 );
  //This seems to default to TZ = 8 hours EAST anyway.
  configTime( TZ_SEC, DST_SEC, timeServer1, timeServer1, timeServer3 );
  //syncTime();
  
  //Setup defaults first - via EEprom. 
  //setupFromEeprom();
  
  // Connect to wifi 
  setup_wifi();                   
  
  //Pins mode and direction setup for i2c on ESP8266-01
  //not necessary for encoder - done in constructor
     
  //I2C setup SDA pin 0, SCL pin 2 on ESP-01
  Wire.begin(0, 2);
  //I2C setup SDA pin 5, SCL pin 4 on ESP-12
  //Wire.begin(5, 5);
  //Wire.begin(5, 4);
  Wire.setClock(100000 );//100KHz target rate
  
  //Setup output pins
  analogWriteRange(1023);
  analogWriteFreq(100);
  for (int i = 0; i < N_PWM; i++)
  {
    pinMode( pwm_pin[i], OUTPUT );
    analogWrite( pwm_pin[i], output );
  }

  //Setup the PID controllers
  //Set the PID to standby until are enabled or explicitly called
  if ( aPidEnable )
    aPid.SetMode(AUTOMATIC);
  else
    aPid.SetMode(MANUAL);
  aPid.SetSampleTime(250);//ms
  aPid.SetOutputLimits(0,1023);
  aPid.SetControllerDirection( DIRECT );//

  aPid.Compute();
  for (int i = 0; i < N_PWM; i++)
  {
    pinMode( pwm_pin[i], OUTPUT );
    analogWrite( pwm_pin[i], output );
  }
  
  Serial.println("Pins setup & interrupts attached.");
    
  //Open a connection to MQTT
  client.setServer( mqtt_server, 1883 );
  client.connect( thisID, pubsubUserID, pubsubUserPwd ); 

  //Create a timer-based callback that causes this device to read the local i2C bus devices for data to publish.
  client.setCallback( callback );
  client.subscribe( inTopic );
  client.subscribe( "/skybadger/sensors/dewpoint/+" );
  Serial.println("MQTT setup complete.");

  //Setup the sensors
  Serial.print("Probe BMP280: ");
  tempSensPresent = bmp280.initialize();
  if ( !tempSensPresent  ) 
  {
    Serial.println("BMP280 Sensor missing");
  }
  else
  {
    Serial.println("BMP280 Sensor found");
    // onetime-measure:
    bmp280.setEnabled(0);
    bmp280.triggerMeasurement();
  }
  
  //Setup webserver handler functions
  //handler code in separate file. 
  server.on("/", handleStatusGet);
  server.onNotFound(handleNotFound); 
  server.on("/heater",                     HTTP_GET, handleHeaterGet );
  server.on("/heater",                     HTTP_PUT, handleHeaterPut );
  server.on("/heater/setup",               HTTP_GET, handleSetup );
  server.on("/heater/setup/hostname",      HTTP_PUT, handleSetupHostnamePut );
  server.on("/heater/setup/temperature",   HTTP_PUT, handleSetupTemperaturePut );
  server.on("/heater/setup/servoing",      HTTP_PUT, handleSetupServoPut );
  server.on("/heater/setup/dewpoint",      HTTP_PUT, handleSetupDewpointTrackPut );  
  
  //Remote wireless reload. 
  httpUpdater.setup(&server);
  Serial.println( "Setup webserver handlers complete");
  server.begin();
    
  //Setup timers for measurement loop
  //setup interrupt-based 'soft' alarm handler for periodic acquisition/recording of new measurements.
  ets_timer_setfn( &timer, onTimer, NULL ); 
  ets_timer_setfn( &timeoutTimer, onTimeoutTimer, NULL );   
  
  //fire timer every 200 msec
  //Set the timer function first
  ets_timer_arm_new( &timer, 200, 1/*repeat*/, 1);
  //ets_timer_arm_new( &timeoutTimer, 2500, 0/*one-shot*/, 1);
  
  Serial.println( "Setup complete" );
}

//Timer handler for 'soft' interrupt timer. 
void onTimer( void * pArg )
{
  newDataFlag = true;
}

//Used to complete timeout actions. 
void onTimeoutTimer( void* pArg )
{
  //Read command list and apply. 
  timeoutFlag = true;
}

//Main processing loop
void loop()
{
  String timestamp;
  String outString;
  static int loopCount = 0;
  
  //DynamicJsonBuffer jsonBuffer(256);
  //JsonObject& root = jsonBuffer.createObject();
  if ( WiFi.status() != WL_CONNECTED )
  {
    device.restart();
  }
  
  //Update measurements as fast as we can to service PID routine. 
  if( newDataFlag == true )
  {
    getTimeAsString( timestamp );
    if( tempSensPresent )
    {
      float temp = 0.0F;
      bmp280.awaitMeasurement();
      bmp280.getTemperature( temp );
      currentTemperature = ( double) temp;
      bmp280.triggerMeasurement();

    }
    aPid.Compute();
    analogWrite( pwm_pin[0], (int) output );
    //TODO Add checks for scaling and limits - bound between 0 and 1024 for 10-bit resolution.
  }

  if( client.connected() )   
  {
    if (callbackFlag == true )
    {    
      //publish results
      publishHeater();
      publishHealth();
      callbackFlag = false;
    }
    client.loop();     
  }
  else
  {  
      reconnectNB(); 
      client.subscribe( inTopic );
      client.subscribe( "/skybadger/sensors/dewpoint/+" );
  }

  //Handle web requests
  server.handleClient();
}

/* MQTT callback for subscription and topic.
 * Only respond to valid states ""
 * Publish under ~/skybadger/sensors/<sensor type>/<host>
 * Note that messages have an maximum length limit of 18 bytes - set in the MQTT header file. 
 */
 void callback(char* topic, byte* payload, unsigned int length) 
 {  
  int index = 0;
  String sTopic = String( topic );
  String sPayload = String( (char*) payload );
  
  //Ex: "/skybadger/sensors/dewpoint/espSEN01"
  index = sTopic.indexOf( "/dewpoint/" );

  DynamicJsonBuffer jsonBuffer(300);

  //DEBUGS1( "Dewpoint found in callback at index: " );DEBUGSL1( index );
  //DEBUGS1( "topic length " );DEBUGSL1( sTopic.length() );
  if ( index >= 0 && index < sTopic.length()  )
  {
    //read dewpoint and update local value + a delta if dewpoint tracking enabled;  
    //DEBUGS1( "Payload: "); DEBUGSL1( sPayload.c_str() );
    //Ex: "{"sensor":"DHT11","dewpoint":4.091456}
    JsonObject& root = jsonBuffer.parseObject( sPayload.c_str() );
    if ( root.success( ) )
      dewpoint = root["dewpoint"];
    
    //Could also check the time of publication to see how current it is.. 
    targetTemperature = dewpoint + delta;
    DEBUGS1( "New target temperature: "); DEBUGSL1( dewpoint + delta);
    //dewpoint = 5.5F; //for testing purposes. Comment out later. 
  }
  //set callback flag
  callbackFlag = true;   
 }

/*
 * Had to do a lot of work to get this to work 
 * Mostly around - 
 * length of output buffer
 * reset of output buffer between writing json strings otherwise it concatenates. 
 * Writing to serial output was essential.
 */
 void publishHeater( void )
 {
  String outTopic;
  String output;
  String timestamp;
  
  //checkTime();
  getTimeAsString2( timestamp );
  
  //publish to our device topic(s)
  DynamicJsonBuffer jsonBuffer(300);
  JsonObject& root = jsonBuffer.createObject();

  output="";//reset
  
  root["target"] = targetTemperature;
  root["time"] = timestamp;
  root["actual"] = currentTemperature;
  //root["device"] = "label";
  outTopic = outSenseTopic;
  outTopic.concat("Heater/");
  outTopic.concat(myHostname);

  root.printTo( output );
  if ( client.publish( outTopic.c_str(), output.c_str(), true ) )        
    Serial.printf( "Published heater status %s to %s\n",  output.c_str(), outTopic.c_str() );
  else    
    Serial.printf( "Failed to publish heater status %s to %s\n",  output.c_str(), outTopic.c_str() );
 }
 
void publishHealth(void)
{
 String outTopic;
  String output;
  String timestamp;
  
  //checkTime();
  getTimeAsString2( timestamp );
  
  //publish to our device topic(s)
  DynamicJsonBuffer jsonBuffer(300);
  JsonObject& root = jsonBuffer.createObject();
    
  //checkTime();
  getTimeAsString2( timestamp );
//Put a separate notice out regarding device health
  //publish to our device topic(s)
  root["time"] = timestamp;
  root["hostname"] = myHostname;
  root["message"] = "Automatic monitoring";
  root.printTo( output );
  outTopic = outHealthTopic;
  outTopic.concat( myHostname );
  
  if ( client.publish( outTopic.c_str(), output.c_str(), true ) )
    Serial.printf( " Published health message: '%s' to %s\n",  output.c_str(), outTopic.c_str() );
  else
    Serial.printf( " Failed to publish health message: '%s' to %s\n",  output.c_str(), outTopic.c_str() );
}

uint32_t inline ICACHE_RAM_ATTR myGetCycleCount()
{
    uint32_t ccount;
    __asm__ __volatile__("esync; rsr %0,ccount":"=a" (ccount));
    return ccount;
}
