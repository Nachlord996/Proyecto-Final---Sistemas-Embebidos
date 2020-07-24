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

#include <stdbool.h>
#include "measurer.h"
#include "freeRTOS/include/FreeRTOS.h"
#include <math.h>
#include "freeRTOS/include/semphr.h"
#include "freeRTOS/include/task.h"
#include "mcc_generated_files/adc1.h"
#include "Modulos_para_modem/SIM808/SIM808.h"
#include "mcc_generated_files/pin_manager.h"
#include "storage.h"
#include "utils.h"
#include <stdio.h>
#include <time.h>
#include "Modulos_para_modem/GPS/GPS.h"
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


static uint16_t result;
static double linearTrans;
static double averageMs;
static uint8_t parteEntera;
static uint8_t decimal;

static TaskHandle_t measureHandle;
static TaskHandle_t ledsHandle;

uint8_t phoneMessage[5];
uint8_t textMessage[80];
uint8_t formattedTime[20];
uint8_t NMEA_FRAME_BUFFER[64];
uint8_t temperature_string[10];
uint8_t gMapsLink[60];

enum measurerMachine {
    WORKING, DONE
} mState;


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
void measureTemp(void *p_param) {
    char msTaken = 0;
    averageMs = 0;
    linearTrans = 0;
    mState = WORKING;
    for (;;) {
        switch (mState) {
            case WORKING:
                result = ADC1_GetConversion(TEMP);
                linearTrans = (double) result * 100 / 1023;
                linearTrans = floor(linearTrans + 0.5);
                linearTrans = ((0.1) * linearTrans) + 32;
                averageMs = averageMs + linearTrans;
                msTaken++;
                if (msTaken == 10) {
                    xSemaphoreGive(semaphore);
                    vTaskDelay(pdMS_TO_TICKS(250));
                    averageMs = averageMs / 10;
                    mState = DONE;
                }
                xSemaphoreGive(semaphore);
                vTaskDelay(pdMS_TO_TICKS(250));
                break;
            case DONE:;
                // Store Temperature Data

                time_t exampleTime;
                GPSPosition_t position;
                bool isFrameValid = false;

                if (get_NMEAFrame(NMEA_FRAME_BUFFER)) {
                    GPS_getPosition(&position, NMEA_FRAME_BUFFER);
                    GPS_getUTC(&time_holder, NMEA_FRAME_BUFFER);
                    exampleTime = mktime(&time_holder);
                    isFrameValid = true;
                } else {
                    exampleTime = INVALID_DATA;
                }

                addRegister(averageMs, &exampleTime, &position);

                floatToString(averageMs, 1, temperature_string);

                // End Storage
                // Send Alert SMS
                if (isPhoneSet) {
                    if (averageMs > *getThreshold()) {
                        if (isFrameValid) {
                            GPS_generateGoogleMaps(gMapsLink, position);
                            strftime(formattedTime, sizeof (formattedTime), "%d/%m/%Y %H:%M:%S", gmtime(&exampleTime));
                            sprintf(textMessage, "%d %s %s %sC %s", *getDeviceID(), formattedTime, gMapsLink, temperature_string, STR_END);
                        } else {
                            sprintf(textMessage, "%d %s %sC %s", *getDeviceID(), INVALID_FRAME_MESSAGE, temperature_string, STR_END);
                        }

                        if (xSemaphoreTake(c_semGSMIsReady, portMAX_DELAY) == pdTRUE) {
                            SIM808_sendSMS(PHONE_NUMBER, textMessage);
                            xSemaphoreGive(c_semGSMIsReady);
                        }
                    }
                }
                // End Send SMS

                // Suicide-Squad
                vTaskDelete((TaskHandle_t) p_param);
                break;
            default:
                break;
        }

    }
}

void manageLEDs(void *p_param) {
    for (;;) {
        xSemaphoreTake(semaphore, pdMS_TO_TICKS(1000));
        switch (mState) {
            case WORKING:
                WS2812_set_Sending_State();
                RGB_LED_eventHandler();
                break;
            case DONE:
                if (averageMs > *getThreshold()) {
                    WS2812_indicateSafeness(true);
                } else {
                    WS2812_indicateSafeness(false);
                }
                RGB_LED_eventHandler();
                vTaskDelay(pdMS_TO_TICKS(2000));
                WS2812_turnOff();

                // Suicide-Squad
                vTaskDelete(ledsHandle);
                break;
            default:
                break;
        }
    }
}


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

bool measuringTasksHandler(bool action) {
    eTaskState measuring;
    eTaskState showing;
    bool deleted = false;


    if (action) {

        xTaskCreate(measureTemp, "measureTemp", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &measureHandle);
        xTaskCreate(manageLEDs, "manageLEDs", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &ledsHandle);

    } else {

        measuring = eTaskGetState(measureHandle);
        showing = eTaskGetState(ledsHandle);

        if (measuring != eDeleted) {
            vTaskDelete(measureHandle);
            deleted = true;
        }

        if (showing != eDeleted) {
            vTaskDelete(ledsHandle);
            deleted = true;
        }

        WS2812_turnOff();
        semaphore = xSemaphoreCreateBinary();
    }

    return deleted;
}


/* *****************************************************************************
 End of File
 */
