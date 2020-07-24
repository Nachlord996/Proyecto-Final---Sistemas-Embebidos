/* ************************************************************************** */
/** GPS

  @Company
    Ucudal

  @File Name
    GPS.c

  @Summary
    Contains the structures and functions to obtain information from GPS frames obtained via SIM808 module
 */
/* ************************************************************************** */

/*******************************************************************************/    
/************************* INCLUDED FILES **************************************/    
/*******************************************************************************/
#include "GPS.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "../../utils.h"

/*******************************************************************************/    
/************************* LOCAL VARIABLES *************************************/    
/*******************************************************************************/     


/*******************************************************************************/    
/************************* INTERFACE FUNCTIONS *********************************/    
/*******************************************************************************/

/**
    @Function
        void GPS_getPosition( GPSPosition_t* pos, uint8_t *sentence )      
      
    @Summary
        Fills the position structure with information from the GPS frame received.
   
    @Remarks
        String must start after +CGNSINF: 
 **/
void GPS_getPosition( GPSPosition_t *p_pos, uint8_t *p_sentence ){   
    uint8_t offset;
    uint8_t *ptr;

    offset=GPS_RMC_RUN_LEN+GPS_RMC_COMMA_LEN+GPS_RMC_FIX_LEN+GPS_RMC_COMMA_LEN+GPS_RMC_UTC_LEN+GPS_RMC_COMMA_LEN + 12;
    double latitude = 0; 
    double longitude = 0;
    sscanf((p_sentence + offset), "%lf", &latitude);
    sscanf((p_sentence + offset + 11), "%lf", &longitude);
    p_pos->latitude = latitude;
    p_pos->longitude = longitude;
}


/**
    @Function
        void GPS_getUTC( struct tm *p_newtime, uint8_t *p_sentence )      
      
    @Summary
        Fills the time structure with TIME information from the GPS frame received.

    @Remarks
        String must start after +CGNSINF: 
 **/
void GPS_getUTC( struct tm *p_newtime, uint8_t *p_sentence ){
    uint8_t offset;
    uint8_t temp_str[5];

    offset=GPS_RMC_RUN_LEN+GPS_RMC_COMMA_LEN+GPS_RMC_FIX_LEN+GPS_RMC_COMMA_LEN + 12;
    // Copy Year YYYY
    memset( temp_str, 0, 5 );
    strncpy( temp_str, (p_sentence+offset), 4 );
    p_newtime->tm_year=atoi( temp_str )-1900;
    offset+=4;
    // Copy Month MM
    memset( temp_str, 0, 5 );
    strncpy( temp_str, (p_sentence+offset), 2 );
    p_newtime->tm_mon = atoi(temp_str)-1;
    offset+=2;
    // Copy Day DD
    memset( temp_str, 0, 5 );
    strncpy( temp_str, (p_sentence+offset), 2 );
    p_newtime->tm_mday = atoi(temp_str);
    offset+=2;
    // Copy Hour HH
    memset( temp_str, 0, 5 );
    strncpy( temp_str, (p_sentence+offset), 2 );
    p_newtime->tm_hour = atoi(temp_str);
    offset+=2;
    // Copy Minutes MM
    memset( temp_str, 0, 5 );
    strncpy( temp_str, (p_sentence+offset), 2 );
    p_newtime->tm_min = atoi(temp_str);
    offset+=2;
    // Copy Seconds SS
    memset( temp_str, 0, 5 );
    strncpy( temp_str, (p_sentence+offset), 2 );
    p_newtime->tm_sec = atoi(temp_str);
}

/**
    @Function
        double GPS_getGroundDistance( GPSPosition_t *a, GPSPosition_t *b )     
      
    @Summary
        Calculates ground distance(in km) between to geographical points. (Uses spherical earth model)

    @Remarks
        None
 **/
double GPS_getGroundDistance( GPSPosition_t *a, GPSPosition_t *b ){
    double angle;
    double lat_a, lon_a, lat_b, lon_b;

    lat_a=a->latitude*M_PI/180;
    lon_a=a->longitude*M_PI/180;
    lat_b=b->latitude*M_PI/180;
    lon_b=b->longitude*M_PI/180;

	angle = sqrt(cos(lat_a)*cos(lat_b)*pow(sin((lon_a - lon_b)/2), 2) + pow(sin((lat_a - lat_b)/2), 2));
	angle = 2*asin(angle);

	return angle * GPS_EARTH_RADIUS;
}

void GPS_generateGoogleMaps( uint8_t *p_linkDest, GPSPosition_t p_gpsData ){
    uint8_t parsedCoord[15];
            
    strcpy( p_linkDest, "http://maps.google.com/?q=" );
    floatToString(p_gpsData.latitude, 7, parsedCoord);
    strcat( p_linkDest, parsedCoord );
    memset(parsedCoord, 0, sizeof (parsedCoord));
    strcat( p_linkDest, "," );
    floatToString(p_gpsData.longitude, 7, parsedCoord);
    strcat( p_linkDest, parsedCoord );
}
