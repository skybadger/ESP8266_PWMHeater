#if !defined _ESP8266_PWMH_Handlers_h_
#define _ESP8266_PWMH_Handlers_h_

  void handleRoot(void);
  void handleNotFound();
  void handleHeaterGet( void );
  void handleStatusGet( void);
  void handleHeaterPut( void );
    
  void handleHeaterGet( void )
  {
    String timeString = "", message = "";

    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    root["servoing"]       = (bool) aPidEnable;
    root["dewpoint"]       = (bool) dewpointTrackState;

    if( tempSensPresent )
    {
      root["temperature"]    = (double) targetTemperature;
    }
    else
      root["Message"]     = "Heater temp sensor not present";

    root.printTo( Serial );Serial.println(" ");
    root.printTo(message);
    server.send(200, "application/json", message);      
  }

  void handleHeaterPut( void )
  {
    String timeString = "", message = "";
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    float newTemp = targetTemperature;
    bool servoing = aPidEnable;
    bool dewpointTracking = dewpointTrackState;
    
    int status = 200;
    
    if( server.hasArg( "temperature" ) || server.hasArg( "servo" ) || server.hasArg( "dewpoint" ) )
    {
       //Update existing with new if valid. 
       if( server.hasArg( "temperature" ) )
       {
          newTemp = (float)server.arg("temperature").toDouble();
          if( newTemp <= 35.0 && newTemp >= 0.0 )
             targetTemperature = newTemp;
       }

       if( server.hasArg( "servo" ) )
       {
          servoing = server.arg("servo").toInt();
          if( servoing == true || servoing == false )
          {
            servoing = ( servoing == 0 )? false: true;
            aPidEnable = servoing;
          }
       }
       
       if( server.hasArg( "dewpoint" ) )
       {
          dewpointTracking = server.arg("dewpoint").toInt();
          if( dewpointTracking == 0 || dewpointTracking == 1 )
          {
            dewpointTracking = ( dewpointTracking == 0 )? false: true;
            dewpointTrackState =  dewpointTracking;
          }
       }
       status = 200;
    }
    else
    {
       status = 401;
    }

    root["time"] = getTimeAsString2( timeString );
    
    if( tempSensPresent )
      root["temperature"]    = (double) targetTemperature;
    root["servoing"]       =  ( aPidEnable == true )? "true": "false";
    root["dewpoint"]       =  ( dewpointTrackState == true )? "true": "false";
    
    root.printTo( message );
    server.send( status, "application/json", message);      
  }
 
 void handleStatusGet( void)
 {
      String timeString = "", message = "";
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();
    //Update the error message handling.
    // JsonArray errors = root.createArray( "errors" );
    
    root["time"] = getTimeAsString( timeString );

    root.printTo( Serial ); Serial.println(" ");
    root.printTo( message );
    server.send(200, "application/json", message);  
 }

   /*
   * Web server handler functions
   */
  void handleNotFound()
  {
  String message = "URL not understood\n";
  message.concat( "Simple status read: http://");
  message.concat( myHostname );
  message.concat ( "\n");
  message.concat( "temperature read: http://");
  message.concat( myHostname );
  message.concat ( "/temperature \n");
  message.concat( "heater?temperature=&serving=&dewpoint= write: http://");
  message.concat( myHostname );
  message.concat ( "/heater \n");

  server.send(404, "text/plain", message);
  }
 
  //Return sensor status
  void handleRoot()
  {
    String timeString = "", message = "";
    DynamicJsonBuffer jsonBuffer(256);
    JsonObject& root = jsonBuffer.createObject();

    root["time"] = getTimeAsString( timeString );
    //Todo add system status here
    
    
    //root.printTo( Serial );
    root.printTo(message);
    server.send(200, "application/json", message);
  }

#endif
