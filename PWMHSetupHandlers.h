/*
File to be included into relevant device REST setup 
*/
//Assumes Use of ARDUINO ESP8266WebServer for entry handlers
#if !defined _PWMHEATERHANDLERS_H_
#define _PWMHEATERHANDLERS_H_

#include "JSONHelperFunctions.h"

//Handlers used for setup webpage.
void handleSetup             ( void );
void handleSetupHostnamePut  ( void );
void handleSetupTemperaturePut ( void );
void handleSetupAutoTrackPut (void);

//Local functions
String& setupFormBuilder( String& htmlForm, String& errMsg );

//Non ASCOM functions
/*
 * String& setupFormBuilder( String& htmlForm )
 */
 void handleSetup(void)
 {
  String output = "";
  String err = "";
  output = setupFormBuilder( output, err );
  server.send(200, "text/html", output );
  return;
 }

//Don't forget MQTT ID aligns with hostname too 
void handleSetupHostnamePut( void ) 
{
  String form;
  String errMsg;
  String newName;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleSetupHostnamePut" );
  
  //throw error message
  if( server.hasArg("hostname"))
  {
    newName = server.arg("hostname");
    DEBUGS1( "new hostname:" );DEBUGSL1( newName );
  }
  if( newName != NULL && newName.length() != 0 &&  newName.length() < MAX_NAME_LENGTH )
  {
    //save new hostname and cause reboot - requires eeprom read at setup to be in place.  
    memcpy( myHostname, newName.c_str(),  min( sizeof(newName.c_str()),(unsigned) MAX_NAME_LENGTH ) );
    memcpy( thisID, newName.c_str(),  min( sizeof(newName.c_str()),(unsigned) MAX_NAME_LENGTH ) );  
    server.send( 200, "text/html", "rebooting!" ); 

    //Write to EEprom
    saveToEeprom();
    device.restart();
  }
  else
  {
    errMsg = "handleSetupHostnamePut: Error handling new hostname";
    DEBUGSL1( errMsg );
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form ); 
  }
}

void handleSetupTemperaturePut( void ) 
{
  String form;
  String errMsg;
  float newTemp = 0.0;
  String newName;

  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleSetupTemperaturePut" );
  
  //throw error message
  if( server.hasArg("temperature"))
  {
    newTemp = (float) server.arg("temperature").toDouble();
    DEBUGS1( "new temperature:" );DEBUGSL1( newTemp );
  }

  if( newTemp >= 0.0 && newTemp <= 50 )
   {  //Write to EEprom
      targetTemperature = newTemp;
      saveToEeprom();
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form ); 
   }
   else
   {
    errMsg = "handleSetupTemperaturePut: Error handling new temperature";
    DEBUGSL1( errMsg );
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form ); 
   }
   DEBUGSL1( "Exiting handleSetupTemperaturePut" );
}

void handleSetupServoPut ( void) 
{
  String form;
  String errMsg;
  int newTrackState = 99;
  bool success = false;
  
  debugURI( errMsg );
  DEBUGSL1 (errMsg);
  DEBUGSL1( "Entered handleSetupServoPut " );
  
  if( server.hasArg("servo"))
  {
    newTrackState = server.arg("servo").toInt();
    DEBUGS1( "new temperature servo state:" );DEBUGSL1( newTrackState );
  }

   if( newTrackState == 0 || newTrackState == 1  )
   {
      errMsg = "";
      aPidEnable = ( newTrackState == 0 )? false : true;
      form = setupFormBuilder( form, errMsg );
      server.send( 200, "text/html", form ); 
   
      //Write to EEprom
      saveToEeprom();
      success = true;
   }

  if ( !success ) 
  {
    DEBUGSL1( errMsg );
    errMsg = "Entered handleSetupServoPut : Error handling new wheel name";
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form ); 
  }
  
  return;
}

void handleSetupDewpointTrackPut ( void) 
{
   String form;
   String errMsg;
   int newTrackState = 99;
   bool success = false;

   debugURI( errMsg );
   DEBUGSL1 (errMsg);
   DEBUGSL1( "Entered handleSetupDewpointTrackPut " );

   if( server.hasArg("dewpoint"))
   {
    newTrackState = server.arg("dewpoint").toInt();
    DEBUGS1( "new temperature tracking state:" );DEBUGSL1( newTrackState );
   }

   if( newTrackState == 0 || newTrackState == 1  )
   {
      errMsg = "";
      dewpointTrackState = ( newTrackState == 0 )? false : true;
      form = setupFormBuilder( form, errMsg );
      server.send( 200, "text/html", form ); 

      //Write to EEprom
      saveToEeprom();
      success = true;
   }

   if ( !success ) 
   {
    DEBUGSL1( errMsg );
    errMsg = "handleSetupDewpointTrackPut : Error handling new dewpoint tracking setting";
    form = setupFormBuilder( form, errMsg );
    server.send( 200, "text/html", form ); 
   }
  
   return;
}

/*
 * Handler for setup dialog - issue call to <hostname>/setup and receive a webpage
 * Fill in form and submit and handler for each form button will store the variables and return the same page.
 optimise to something like this:
 https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/
 Bear in mind HTML standard doesn't support use of PUT in forms and changes it to GET so arguments get sent in plain sight as 
 part of the URL.
 */
String& setupFormBuilder( String& htmlForm, String& errMsg )
{ 
  htmlForm = "<!DocType html><html lang=\"en\" ><head></head><meta></meta><body>\n";
  if( errMsg != NULL && errMsg.length() > 0 ) 
  {
    htmlForm +="<div bgcolor='A02222'><b>";
    htmlForm.concat( errMsg );
    htmlForm += "</b></div>";
  }
  //Hostname
  htmlForm += "<h1> Enter new hostname for Heater interface</h1>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/heater/setup/hostname\" method=\"PUT\" id=\"hostname\" >\n";
  htmlForm += "Changing the hostname will cause the device to reboot and may change the address!\n<br>";
  htmlForm += "<input type=\"text\" name=\"hostname\" value=\"";
  htmlForm.concat( myHostname );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form>\n";
  
  //Set-point Temperature
  htmlForm += "<h1> Enter new set-point temperature for heater </h1>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat( myHostname );
  htmlForm += "/heater/setup/temperature\" method=\"PUT\" id=\"temperature\" >\n";
  htmlForm += "<input type=\"number\" name=\"temperature\" value=\"";
  htmlForm.concat( targetTemperature );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form>\n";
  
  //Dewpoint tracking
  htmlForm += "<h1> Enable dewpoint tracking </h1>\n";
  htmlForm += "<p> For dewpoint tracking, you must have a sensor reporting dewpoint temperatures to your MQTT server at /skybadger/sensor/dewpoint </p>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/heater/setup/dewpoint\" method=\"PUT\" id=\"dewpoint\" >\n";
  htmlForm += "<input type=\"check\" name=\"dewpoint\" value=\"";
  htmlForm.concat( (dewpointTrackState)? "true":"false" );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form>\n";  

  //Setpoint tracking
  htmlForm += "<h1> Enable temperature set-point servo tracking</h1>\n";
  htmlForm += "<form action=\"http://";
  htmlForm.concat(myHostname);
  htmlForm += "/heater/setup/servo\" method=\"PUT\" id=\"servo\" >\n";
  htmlForm += "<input type=\"check\" name=\"servo\" value=\"";
  htmlForm.concat( (aPidEnable )? "true":"false" );
  htmlForm += "\">\n";
  htmlForm += "<input type=\"submit\" value=\"submit\">\n</form>\n";  
  
  htmlForm += "</body>\n</html>\n";

 return htmlForm;
}
#endif
