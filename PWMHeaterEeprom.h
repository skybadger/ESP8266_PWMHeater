/* File to define the eeprom variable save/restore operations for the PWM wireless servo heater controller */
#ifndef _PWMHEATER_EEPROM_H_
#define _PWMHEATER_EEPROM_H_

#include "ProjectDefs.h"


//definitions
void setDefaults(void );
void saveToEeprom(void);
void setupFromEeprom(void);

void setDefaults( void )
{
  DEBUGSL1( "Eeprom setDefaults: entered");
  if( myHostname != nullptr )
    {
      free(myHostname);
      myHostname = (char* )calloc( sizeof (char), MAX_NAME_LENGTH );
      strncpy( myHostname, defaultHostname, strlen( defaultHostname) );
    }
  if( thisID != nullptr )
  {
    free(thisID);
    thisID = (char* )calloc( sizeof (char), MAX_NAME_LENGTH );
    strncpy( thisID, defaultHostname, strlen( defaultHostname) );
  }
  
  targetTemperature = 20.0;
  aPidEnable = false;
  dewpointTrackState = false;

 DEBUGSL1( "setDefaults: exiting" );
}

void saveToEeprom( void )
{
  int eepromAddr = 0;

  DEBUGSL1( "savetoEeprom: Entered ");
  
  //Magic number write for first time. 
  EEPROM.put( eepromAddr, magic );
  eepromAddr += sizeof( magic );
  
  //hostname
  EEPROM.put( eepromAddr, myHostname );
  DEBUGS1( "Written hostname: ");DEBUGSL1( myHostname );
  eepromAddr += MAX_NAME_LENGTH;
  
  //
  EEPROM.put( eepromAddr, targetTemperature );
  eepromAddr += sizeof(targetTemperature);  
  DEBUGS1( "Written position: ");DEBUGSL1( targetTemperature );
    
  EEPROM.put( eepromAddr, aPidEnable );
  eepromAddr += sizeof( aPidEnable );

  EEPROM.put( eepromAddr, dewpointTrackState );
  eepromAddr += sizeof( dewpointTrackState );
    
  EEPROM.commit();
  DEBUGSL1( "saveToEeprom: exiting ");
}

void setupFromEeprom( void )
{
  int eepromAddr = 0;
  char* newHostname = nullptr;
  int testMagic = 0x00;
    
  DEBUGSL1( "setUpFromEeprom: Entering ");
  
  //Setup internal variables - read from EEPROM.
  EEPROM.get( eepromAddr = 0, testMagic );
  
  DEBUGS1( "Read magic: ");DEBUGSL1( testMagic );
  if ( ( int ) testMagic != magic ) //initialise
  {
    setDefaults();
    saveToEeprom();
    DEBUGSL1( "Failed to find init magic byte - wrote defaults ");
    return;
  }    
  eepromAddr += sizeof ( byte );
      
  //hostname - directly into variable array 
  if( myHostname != nullptr )
  {
    free (myHostname);
    newHostname = (char*) calloc( sizeof( char) , MAX_NAME_LENGTH );
  }
    
  EEPROM.get( eepromAddr, newHostname );
  
  eepromAddr  += MAX_NAME_LENGTH;
  myHostname = newHostname;
  DEBUGS1( "Read hostname: ");DEBUGSL1( myHostname );

  EEPROM.get( eepromAddr, targetTemperature );
  eepromAddr += sizeof( targetTemperature );

  EEPROM.get( eepromAddr, aPidEnable );
  eepromAddr += sizeof( aPidEnable );

  EEPROM.get( eepromAddr, dewpointTrackState );
  eepromAddr += sizeof( dewpointTrackState );
  DEBUGSL1( "setupFromEeprom: exiting" );
}
#endif
