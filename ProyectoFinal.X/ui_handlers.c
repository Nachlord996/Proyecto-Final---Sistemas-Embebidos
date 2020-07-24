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
#include "ui_handlers.h"
#include <stdbool.h>
#include <stdint.h>
#include "storage.h"

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

bool pressAnyKey(uint8_t* data, uint8_t length) {
    return true;
}

bool configID(uint8_t* data, uint8_t length) {
    char i;
    bool result = true;
    bool needComp = false;
    if (length > strlen(MAX_VALUE)) {
        result = false;
    } else {
        if (length == strlen(MAX_VALUE)) {
            needComp = true;
        }
        
        for (i = 0; i < length; i++) {
            if (!isdigit(data[i])) {
                result = false;
                break;
            }
            if (needComp && (data[i] > MAX_VALUE[i])) {
                result = false;
                break;
            }
        }

        if (result) {
            sscanf(data, "%d", getDeviceID());
        }
    }
    return result;
}

bool configThreshold(uint8_t* data, uint8_t length) {
    bool result = false;
    if (length == 4 && data[2] == '.') {
        if (isdigit(data[0]) && isdigit(data[1]) && isdigit(data[3])) {
            sscanf((char *) data, "%f", getThreshold());
            result = true;
        }
    }
    return result;
}

bool configPhoneNumber(uint8_t* data, uint8_t length) {
  
    bool result = false;
      
    if (length == 9 && data[0] == '0') {
        result = true;
        char i;
        for (i = 0; i < length; i++){
            if (!isdigit(data[i])){
                result = false;
                break;
            }
        }
       
        if (result){
            sprintf(PHONE_NUMBER,"\"%s\"", data);
            isPhoneSet = true;
        }
    }
     
    return result;
}




/* *****************************************************************************
 End of File
 */
