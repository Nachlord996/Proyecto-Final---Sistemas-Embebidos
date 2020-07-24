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
#include "show_ui.h"
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "storage.h"
#include "ui_handlers.h"
#include "freeRTOS/portable/../include/FreeRTOS.h"
#include "mcc_generated_files/usb/usb_device_cdc.h"
#include "freeRTOS/include/semphr.h"

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

static uint8_t Input_Buffer[30];
static uint8_t Log_Register_Buffer[80];

static char current_index = 0;
static enum pendingMessage state = INIT;
enum pendingMessage previous_State = WAITING;
bool(*handler)(uint8_t*, uint8_t) = &pressAnyKey;

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

static void sendNewLine() {
    putUSBUSART(newLine, sizeof (newLine));
}

static bool initFunc(uint8_t* data, uint8_t length) {
    bool result = false;

    if (length == 1) {
        result = true;
        switch (data[0]) {
            case '1':
                state = CONFIG_ID;
                break;
            case '2':
                state = CONFIG_THRESHOLD;
                break;
            case'3':
                state = CONFIG_PHONE_NUMBER;
                break;
            case '4':
                state = GET_LOG;
                break;
            case'5':
                state = DELETE_LOG;
                break;
            default:
                result = false;
                break;
        }
    }

    return result;
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



void checkUSBStatus(void *p_param) {
    for (;;) { 
        CDCTxService();
        if ((USBGetDeviceState() >= CONFIGURED_STATE) && !USBIsDeviceSuspended()) {
            xSemaphoreGive(semaphoreUSB);
        }
      
    }
}

void showMenu(void *p_param) {
    for (;;) {
        xSemaphoreTake(semaphoreUSB, pdMS_TO_TICKS(1200));
        showInterface();
    }
}

void showInterface() {
    switch (state) {
        case INIT:
            putUSBUSART(greeting, sizeof (greeting));
            state = WAITING;
            break;
        case SHOW_MENU:
            putUSBUSART(choices, sizeof (choices));
            previous_State = SHOW_MENU;
            state = WAITING;
            break;
        case WAITING:
            memset(Input_Buffer, 0, sizeof (Input_Buffer));
            uint8_t numBytes = getsUSBUSART(Input_Buffer, sizeof (Input_Buffer));
            if (numBytes > 0) {
                sendNewLine();
                if (handler(Input_Buffer, numBytes)) {
                    handler = &initFunc;
                    if (previous_State != SHOW_MENU) {
                        state = SHOW_MENU;
                    }
                } else {
                    state = previous_State;
                }
            }
            break;
        case CONFIG_ID:;
            putUSBUSART(enterID, sizeof (enterID));
            state = WAITING;
            previous_State = CONFIG_ID;
            handler = &configID;
            break;
        case CONFIG_THRESHOLD:
            putUSBUSART(enterThreshold, sizeof (enterThreshold));
            state = WAITING;
            previous_State = CONFIG_THRESHOLD;
            handler = &configThreshold;
            break;
        case CONFIG_PHONE_NUMBER:
            putUSBUSART(enterPhoneNumber, sizeof (enterPhoneNumber));
            state = WAITING;
            previous_State = CONFIG_PHONE_NUMBER;
            handler = &configPhoneNumber;
            break;
        case GET_LOG:
            if (getRegister(current_index, Log_Register_Buffer)) {
                state = SHOW_MENU;
                current_index = -1;
            } else{
                putUSBUSART(Log_Register_Buffer, sizeof (Log_Register_Buffer));
            }
            current_index++;
            break;
        case DELETE_LOG:
            initializeStorage();
            putUSBUSART(logHasReset, sizeof (logHasReset));
            state = SHOW_MENU;
            break;
        default:
            break;
    }
}

/* *****************************************************************************
 End of File
 */
