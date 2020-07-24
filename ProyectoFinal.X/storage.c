/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */
#include "storage.h"
#include <stdbool.h>
#include "freeRTOS/include/FreeRTOS.h"
#include "mcc_generated_files/rtcc.h"
#include <stdlib.h>
#include "utils.h"
#include "measurer.h"
#include <stdio.h>

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */
/** Descriptive Data Item Name

  @Summary
    Brief one-line summary of the data item.
    
  @Description
    Full description, explaining the purpose and usage of data item.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
  @Remarks
    Any additional remarks
 */
static measure_register Log_Storage[STORAGE_ROOM];
static char HEAD = 0;

static float THRESHOLD = 37.0;
static uint32_t DEVICE_ID = 0;


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* ************************************************************************** */

/** 
  @Function
    int ExampleLocalFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Description
    Full description, explaining the purpose and usage of the function.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.

  @Precondition
    List and describe any required preconditions. If there are no preconditions,
    enter "None."

  @Parameters
    @param param1 Describe the first parameter to the function.
    
    @param param2 Describe the second parameter to the function.

  @Returns
    List (if feasible) and describe the return values of the function.
    <ul>
      <li>1   Indicates an error occurred
      <li>0   Indicates an error did not occur
    </ul>

  @Remarks
    Describe any special behavior not described above.
    <p>
    Any additional remarks.

  @Example
    @code
    if(ExampleFunctionName(1, 2) == 0)
    {
        return 3;
    }
 */


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

// *****************************************************************************

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
void initializeStorage() {
    int i;
    for (i = 0; i < STORAGE_ROOM; i++) {
        Log_Storage[i] = (measure_register){.TIME = NONE};
    }
    HEAD = 0;
}

void addRegister(float temp, time_t* time, GPSPosition_t* position) {
    if (HEAD < STORAGE_ROOM) {
        uint8_t integerPart = (uint8_t) temp;
        float decimalPart = temp - integerPart;
        uint8_t cut = (uint8_t) (10 * decimalPart);
        Log_Storage[HEAD] = (measure_register){.POSITION = *position, .TEMPERATURE_INTEGER = integerPart, .TEMPERATURE_DECIMAL = cut, .TIME = *time};
        HEAD++;
    }
}

void updateTime(struct tm* time) {
    RTCC_TimeGet(time);
    time->tm_isdst = -1;
}

bool getRegister(uint8_t position, uint8_t* buffer) {
    bool end = false;
    
    if (position >= STORAGE_ROOM) {
        return true;
    }
    
    time_t dnt = Log_Storage[position].TIME;

    if (dnt == NONE) {
        return true;
    }

    uint8_t lat[10];
    uint8_t lon[10];
    uint8_t time[20];

    uint8_t temp_int = Log_Storage[position].TEMPERATURE_INTEGER;
    uint8_t temp_dec = Log_Storage[position].TEMPERATURE_DECIMAL;

    if (dnt != INVALID_DATA) {
        float latitude = Log_Storage[position].POSITION.latitude;
        floatToString(latitude, 6, lat);
        float longitude = Log_Storage[position].POSITION.longitude;
        floatToString(longitude, 6, lon);
        strftime(time, sizeof (time), "%d/%m/%Y %H:%M:%S", gmtime(&dnt));
        sprintf(buffer, "%d - %d.%d°C - (%s,%s) - %s\n", position, temp_int, temp_dec, lat, lon, time);
    } else {
        sprintf(buffer, "%d - %d.%d°C - %s\n", position, temp_int, temp_dec, INVALID_FRAME_MESSAGE);
    }
    
    return end;
}

void checkStorageExpiration(void *p_param) {
    for (;;) {
        int previous_day = time_holder.tm_mday;
        vTaskDelay(pdMS_TO_TICKS(THIRTY_SECONDS));
        updateTime(&time_holder);
        int today = time_holder.tm_mday;

        if (previous_day != today) {
            initializeStorage();
        }
    }
}

float* getThreshold() {
    return &THRESHOLD;
}

uint8_t* getPhoneNumber() {
    return &PHONE_NUMBER;
}

uint32_t* getDeviceID() {
    return &DEVICE_ID;
}


/* *****************************************************************************
 End of File
 */
