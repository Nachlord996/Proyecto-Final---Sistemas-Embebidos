/* ************************************************************************** */
/** SIM808

  @Company
    UCUDAL

  @File Name
    SIM808.h

  @Summary
    Contains datatypes and functions for the communication with the SIM module
  
  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef SIM808_H    /* Guard against multiple inclusion */
#define SIM808_H


/*******************************************************************************/
/************************* INCLUDED FILES **************************************/
/*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "../../freeRTOS/include/FreeRTOS.h"
#include "../../freeRTOS/include/semphr.h"
#include "../../freeRTOS/include/task.h"


/*******************************************************************************/
/************************* CONSTANTS *******************************************/
/*******************************************************************************/
#define SIM808_ERRORS_TO_NO_MODEM           5

/*----------------------- BUFFER LENGTHS ----------------------------------------*/
#define SIM808_RX_BUF_SIZE      256
#define SIM808_TX_BUF_SIZE      256

/*----------------------- GENERAL DATA ----------------------------------------*/
#define SIM808_MAX_RETRIES                  3

/*-----------------------  TIMEOUTS -------------------------------------------*/
#define SIM808_UART_BYTES_TIMEOUT_ms        25
#define SIM808_UART_TIMEOUT_ms              3000
#define SIM808_UART_RETRY_TIMEOUT_ms        250
#define SIM808_TURN_ON_GSM_TIMEOUT_ms       1000
#define SIM808_TURN_ON_GPS_TIMEOUT_ms       1000
#define SIM808_SMS_TIMEOUT_ms               10000

/*----------------------- GPS CONSTANTS ----------------------------------------*/
#define SIM808_GPS_COORD_LENGTH         21

/*----------------------- AT COMMANDS ----------------------------------------*/
//BASIC
#define ATREQ_A                         "A"
#define ATREQ_AT                        "AT" //OK
#define ATREQ_ECHO_OFF                  "ATE0" //RES: ATE0 ... OK
//GSM CONFIG 
#define ATREQ_PIN_QUERY                 "AT+CPIN?"
#define ATREQ_PIN_INPUT                 "AT+CPIN=\""  //para cualquier PIN
#define ATREQ_NTWRK_REG                 "AT+CREG=1"  //OK
#define ATREQ_NTWRK_REG_CHK             "AT+CREG?"  //CREG: 1,1

#define ATREQ_OP_MOVISTAR               "AT+COPS=1,2,\"74807\""
#define ATREQ_OP_ANTEL                  "AT+COPS=1,2,\"74801\""
#define ATREQ_OP_CLARO                  "AT+COPS=1,2,\"74810\""
//GPS CONFIG
#define ATREQ_GPS_PWR_ON                "AT+CGNSPWR=1" //OK
#define ATREQ_GPS_PWR_OFF               "AT+CGNSPWR=0" //OK
#define ATREQ_GPS_PWR_STATUS            "AT+CGNSPWR?" //+CGNSPWR: 0 ó 1  OK
#define ATREQ_GPS_SEQ_RMC               "AT+CGNSSEQ=\"RMC\"" //OK
#define ATREQ_GPS_URC                   "AT+CGNSURC=" //OK
#define ATREQ_GPS_INFO                  "AT+CGNSINF" 
//SMS
#define ATREQ_SMS_FORMAT                "AT+CMGF=1"
#define ATREQ_SMS_SEND_HDR              "AT+CMGS="
#define ATREQ_SMS_SEND_FTR              0x1A

/*----------------------- POSSIBLE ANSWERS ----------------------------------------*/
//BASIC
#define ATRES_OK                        "OK"
//GSM CONFIG
#define ATRES_PIN                       "CPIN:"
#define ATRES_PIN_READY                 "CPIN: READY"
#define ATRES_SIM_PIN                   "CPIN: SIM PIN"
#define ATRES_NTWRK_OK                  "CREG: 1,1"
//GPS
#define ATRES_GPS_INFO                  "+CGNSINF:"
//SMS
#define ATRES_SMS_SEND_PROMPT           ">"


#define SMS_MIN_PHONE_LENGTH            8

/*******************************************************************************/
/************************* GLOBAL VARIABLES *************************************/
/*******************************************************************************/
extern TaskHandle_t modemInitHandle;

extern SemaphoreHandle_t c_semGPSIsReady;
extern SemaphoreHandle_t c_semGSMIsReady;

/*******************************************************************************/
/************************* DATA TYPES ******************************************/
/*******************************************************************************/


/*******************************************************************************/
/************************* INTERFACE FUNCTIONS *********************************/
/*******************************************************************************/

void SIM808_taskCheck( void* p_param );

void SIM808_initModule( void *p_param );

int8_t SIM808_sendSMS( uint8_t *p_phoneNr, uint8_t *p_string );

int8_t SIM808_getNMEA( uint8_t *p_dest );

bool SIM808_validateNMEAFrame( uint8_t *p_frame );

bool get_NMEAFrame(uint8_t *buffer);

#endif /* SIM808_H */

/* *****************************************************************************
 End of File
 */
