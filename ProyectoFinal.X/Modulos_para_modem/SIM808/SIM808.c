/* ************************************************************************** */
/** SIM808

  @Company
    UCUDAL

  @File Name
    SIM808.c

  @Summary
    Contiene las funciones para el manejo del módulo SIM808

 */
/* ************************************************************************** */


/*******************************************************************************/
/************************* INCLUDED FILES **************************************/
/*******************************************************************************/

#include "SIM808.h"

#include "../../mcc_generated_files/pin_manager.h"
#include "../../mcc_generated_files/uart1.h"
#include <string.h>


/*******************************************************************************/
/************************* LOCAL VARIABLES *************************************/
/*******************************************************************************/
static SemaphoreHandle_t c_semIsModuleOnAndReady; /** Indica si el módulo está encendido y respondió OK al AT */
static SemaphoreHandle_t c_semRxIsDataAvailable; /** Indica que llegaron datos para leer por la UART */
static SemaphoreHandle_t c_semTxReady; /** Indica que se puede transmitir por la UART */
SemaphoreHandle_t c_semGPSIsReady; /** Indica que ya se encendió e inicializó el GPS */
SemaphoreHandle_t c_semGSMIsReady; /** Indica que ya se encendió e inicializó el GSM */

static TaskHandle_t initGSMHandle = NULL;
static TaskHandle_t initGPSHandle = NULL;
TaskHandle_t modemInitHandle = NULL;

static uint8_t c_simRxBuffer[SIM808_RX_BUF_SIZE];
static uint8_t c_simTxBuffer[SIM808_TX_BUF_SIZE];


/******************************************************************************/
/************************* LOCAL FUNCTIONS ************************************/
/******************************************************************************/

/** 
  @Function
    bool SIM808_powerOn( void ) 

  @Summary
    Enciende el módulo a través del pin PWRKEY y espera que STATUS sea 1
 */
static bool SIM808_powerOn(void) {
    uint8_t pwrOn_retries = SIM808_MAX_RETRIES;

    while (pwrOn_retries > 0) {
        GPRS_PWRKEY_SetDigitalOutput();
        GPRS_PWRKEY_SetLow();
        vTaskDelay(pdMS_TO_TICKS(1100));
        GPRS_PWRKEY_SetDigitalInput();
        vTaskDelay(pdMS_TO_TICKS(2100));
        if (GPRS_STATUS_GetValue() != 0) {
            return true;
        } else {
            pwrOn_retries--;
        }
    }
    return false;
}

/** 
 * @Function
 *    int8_t SIM808_sendATRequest(uint8_t *p_cmd) 
 *
 * @Summary
 *   Envía un comando AT al módulo
 * 
 * @Param:  p_cmd: Puntero al comando a enviar
 *          p_includeCR: Indica si se quiere agregar el CR al final o no.
 */
static int8_t SIM808_sendATCmd(uint8_t *p_cmd, bool p_includeCR) {
    if (c_semTxReady != NULL) {
        if (xSemaphoreTake(c_semTxReady, portMAX_DELAY) == pdTRUE) {
            UART1_WriteBuffer(p_cmd, strlen(p_cmd));
            if (p_includeCR == true) {
                UART1_Write('\r');
            }
            return true;
        } else {
            return -1; // Timeout
        }
    }
    return -2; // Semáforo no creado
}

/** 
 * @Function
 *   int16_t SIM808_readATResponse(uint32_t p_timeout) 
 *
 * @Summary
 *   Espera un tiempo pasado como parámetro para recibir una respuesta del módulo
 */
static int16_t SIM808_readATResponse(uint32_t p_timeoutms) {
    uint32_t bytesQty;

    if (c_semRxIsDataAvailable != NULL) {
        if (xSemaphoreTake(c_semRxIsDataAvailable, pdMS_TO_TICKS(p_timeoutms)) == pdTRUE) {
            // Doy tiempo de que lleguen todos los datos
            vTaskDelay(pdMS_TO_TICKS(SIM808_UART_BYTES_TIMEOUT_ms));
            memset(c_simRxBuffer, 0, sizeof (c_simRxBuffer));
            if (UART1_ReceiveBufferIsEmpty() == false) {
                bytesQty = UART1_ReadBuffer(c_simRxBuffer, bytesQty);
                return bytesQty; // Retorno cantidad de bytes leídos
            }
            // Llegó cero byte
            return -3;
        } else {
            return -1; // Timeout
        }
    }
    return -2; // Semáforo no creado
}

/**
 * @Function
 *  fsm8_t SIM808_exchangeCmd(uint8_t* p_cmd2Send, uint8_t* p_cmd2Expect, uint32_t p_timeout) 
 *
 * @Summary
 *   Envía un comando y compara contra una respuesta esperada
 * 
 * @Param:  p_cmd2Send: Puntero al comando a enviar
 *          p_answer2Expect: Puntero al comando a esperar como respuesta
 *          p_timeout: Timeout de espera para recibir la respuesta
 *          p_retries: Cantidad de reintentos
 */
static int8_t SIM808_exchangeCmd(uint8_t *p_cmd2Send, uint8_t *p_answer2Expect, uint32_t p_timeout, uint8_t p_retries) {
    int16_t readBytes;
    int8_t retValue;

    // Si no hay nada para mandar solamente espero una respuesta
    do {
        if (p_cmd2Send != NULL) {
            SIM808_sendATCmd(p_cmd2Send, true);
        }
        readBytes = SIM808_readATResponse(p_timeout);
        if (readBytes > 0) {
            if (strstr(c_simRxBuffer, p_answer2Expect) != 0) {
                retValue = true;
                break; // Corto el while
            } else {
                retValue = -2; // Respuesta buscada no encontrada
            }
        } else if (readBytes == -1) {
            retValue = -1; // Timeout
        }
        vTaskDelay(pdMS_TO_TICKS(SIM808_UART_RETRY_TIMEOUT_ms));
    } while (--p_retries > 0);

    return retValue;
}

/**
 * @Function
 *  void SIM808_initGSM( void* p_param ) 
 *
 * @Summary
 *   Tarea que iniciliza el módulo GSM
 */
static void SIM808_initGSM(void* p_param) {
    int8_t resultExchange;

    while (1) {
        if (xSemaphoreTake(c_semIsModuleOnAndReady, pdMS_TO_TICKS(SIM808_TURN_ON_GSM_TIMEOUT_ms)) == pdTRUE) {
            if ((resultExchange = SIM808_exchangeCmd(ATREQ_PIN_QUERY, ATRES_PIN_READY, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
                if ((resultExchange = SIM808_exchangeCmd(ATREQ_OP_ANTEL, ATRES_OK, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
                    if ((resultExchange = SIM808_exchangeCmd(ATREQ_NTWRK_REG, ATRES_OK, SIM808_UART_TIMEOUT_ms, 3 * SIM808_MAX_RETRIES)) == true) {
                        if ((resultExchange = SIM808_exchangeCmd(ATREQ_NTWRK_REG_CHK, ATRES_NTWRK_OK, SIM808_UART_TIMEOUT_ms, 3 * SIM808_MAX_RETRIES)) == true) {
                            if ((resultExchange = SIM808_exchangeCmd(ATREQ_SMS_FORMAT, ATRES_OK, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
                                // Estoy registrado en la red y ya puedo enviar mensajes
                                xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
                                xSemaphoreGive(c_semGSMIsReady); /** Habilito el uso de GSM */
                                vTaskDelete(initGSMHandle);
                            } else {
                                // Fallo el seteo de modo texto
                                xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
                            }
                        } else {
                            // Falló registro en la red
                            xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
                        }
                    } else {

                    }
                } else {
                    // Falló el seteo de operadora
                    xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
                }
            } else {
                // Se necesita pin o falla
                xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
            }
        }
        // No está prendido el modem  
    }
}

/**
 * @Function
 *  void SIM808_initGPS( void* p_param ) 
 *
 * @Summary
 *   Tarea que iniciliza el módulo GSM
 */
static void SIM808_initGPS(void* p_param) {
    int8_t resultExchange;

    while (1) {
        if (xSemaphoreTake(c_semIsModuleOnAndReady, pdMS_TO_TICKS(SIM808_TURN_ON_GSM_TIMEOUT_ms)) == pdTRUE) {
            if ((resultExchange = SIM808_exchangeCmd(ATREQ_GPS_PWR_ON, ATRES_OK, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
                if ((resultExchange = SIM808_exchangeCmd(ATREQ_GPS_SEQ_RMC, ATRES_OK, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
                    // Se prendió y seteó la trama RMC
                    xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
                    xSemaphoreGive(c_semGPSIsReady); /** Habilito el uso de GPS */
                    vTaskDelete(initGPSHandle);
                } else {
                    // Falló el seteo de trama
                    xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
                }
            } else {
                // No prendió el GPS
                xSemaphoreGive(c_semIsModuleOnAndReady); /** Libero el semáforo para que otro pueda hablarle al módulo */
            }
        }
        // No está prendido el modem
    }
}


/******************************************************************************/
/*************** INTERFACE FUNCTIONS ******************************************/
/******************************************************************************/

/**
 * @Function
 *  void SIM808_taskCheck( void* p_param )
 *
 * @Summary
 *   Tarea que iniciliza el módulo SIM808 y lo vuelve a hacer si detecta que se apagó
 */
void SIM808_taskCheck(void* p_param) {

    c_semIsModuleOnAndReady = xSemaphoreCreateBinary();
    c_semTxReady = xSemaphoreCreateBinary();
    c_semRxIsDataAvailable = xSemaphoreCreateBinary();
    c_semGPSIsReady = xSemaphoreCreateBinary();
    c_semGSMIsReady = xSemaphoreCreateBinary();

    while (1) {
        if (GPRS_STATUS_GetValue() == 1) {
            // Si no está el módulo encendido tampoco envío ni recibo
            if (UART1_IsTxDone()) {
                xSemaphoreGive(c_semTxReady);
            }
            if (UART1_IsRxReady()) {
                xSemaphoreGive(c_semRxIsDataAvailable);
            }
        } else {
            // Deshabilito todos los semáforos, si los hubiera libres
            xSemaphoreTake(c_semIsModuleOnAndReady, pdMS_TO_TICKS(10));
            xSemaphoreTake(c_semTxReady, pdMS_TO_TICKS(10));
            xSemaphoreTake(c_semRxIsDataAvailable, pdMS_TO_TICKS(10));
            xSemaphoreTake(c_semGPSIsReady, pdMS_TO_TICKS(10));
            xSemaphoreTake(c_semGSMIsReady, pdMS_TO_TICKS(10));
        }
    }
}

/**
 * @Function
 *  void SIM808_initModule( void *p_param )
 *
 * @Summary
 *   Tarea que iniciliza el módulo SIM808
 */
void SIM808_initModule(void *p_param) {
    int8_t resultExchange;

    while (1) {
        if (SIM808_powerOn() == true) {
            if ((resultExchange = SIM808_exchangeCmd(ATREQ_AT, ATRES_OK, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
                if ((resultExchange = SIM808_exchangeCmd(ATREQ_ECHO_OFF, ATRES_OK, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
                    xSemaphoreGive(c_semIsModuleOnAndReady);
                    xTaskCreate(SIM808_initGSM, "iniGSM", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, &initGSMHandle);
                    xTaskCreate(SIM808_initGPS, "iniGPS", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, &initGPSHandle);
                    vTaskDelete(modemInitHandle);
                } else {
                    // Falla respuesta a deshabilitación de eco
                }
            } else {
                // No se recibe respuesta a AT
            }
        } else {
            // Si no está prendido no puedo hacer nada
        }
    }
}

/**
 * @Function
 *  int8_t SIM808_sendSMS( uint8_t *p_phoneNr, uint8_t *p_string )
 *
 * @Summary
 *   Función que envía en SMS
 * 
 * @Param:  p_phoneNr: String con el número de teléfono entre comillas (Ej: "099123456")
 *          p_string: Cuerpo del mensaje a enviar
 */
int8_t SIM808_sendSMS(uint8_t *p_phoneNr, uint8_t *p_string) {
    int8_t resultExchange;
    int8_t retValue = 0;
    int16_t readBytes;
    uint8_t auxBuffer[SIM808_TX_BUF_SIZE];

    if (strlen(p_phoneNr) < SMS_MIN_PHONE_LENGTH) {
        return -5;
    }
    memset(auxBuffer, 0, sizeof (auxBuffer));
    strcpy(auxBuffer, ATREQ_SMS_SEND_HDR);
    strcat(auxBuffer, p_phoneNr);
    if ((resultExchange = SIM808_sendATCmd(auxBuffer, true)) == true) {
        readBytes = SIM808_readATResponse(SIM808_UART_TIMEOUT_ms);
        if (readBytes > 0) {
            if (strstr(c_simRxBuffer, ATRES_SMS_SEND_PROMPT) != 0) {
                memset(auxBuffer, 0, sizeof (auxBuffer));
                auxBuffer[0] = ATREQ_SMS_SEND_FTR;
                if ((resultExchange = SIM808_sendATCmd(p_string, false)) == true) {
                    if ((resultExchange = SIM808_sendATCmd(auxBuffer, false)) == true) {
                        vTaskDelay(pdMS_TO_TICKS(3000));
                        readBytes = SIM808_readATResponse(SIM808_SMS_TIMEOUT_ms);
                        if (readBytes > 0) {
                            if (strstr(c_simRxBuffer, ATRES_OK) != 0) {
                                retValue = true;
                            } else {
                                retValue = -4; // No se envió
                            }
                        } else if (readBytes == -1) {
                            retValue = -1; // Timeout
                        }
                    } else {
                        // Fallo envío de 0x1A
                        retValue = -2;
                    }
                } else {
                    // Fallo envío de cuerpo de mensaje
                    retValue = -2;
                }
            } else {
                retValue = -3; // Respuesta buscada no encontrada
            }
        } else if (readBytes == -1) {
            retValue = -1; // Timeout
        }
    } else {
        //Falló envío
        retValue = -2;
    }
    return retValue;
}

/**
 * @Function
 *  int8_t SIM808_getNMEA( uint8_t *p_dest )
 *
 * @Summary
 *   Función que obtiene una trama de GPS
 * 
 * @Param:  p_dest: Buffer en el cual se guarda el resultado en caso de devolver true
 */
int8_t SIM808_getNMEA(uint8_t *p_dest) {
    int8_t resultExchange;

    if ((resultExchange = SIM808_exchangeCmd(ATREQ_GPS_INFO, ATRES_OK, SIM808_UART_TIMEOUT_ms, SIM808_MAX_RETRIES)) == true) {
        strcpy(p_dest, c_simRxBuffer);
        return true;
    } else {
        // No se recibe respuesta a AT
        return false;
    }
}

/**
 * @Function
 *  bool SIM808_validateNMEAFrame( uint8_t *p_frame )
 *
 * @Summary
 *   Función que indica si la trama GPS obtenida es válida
 * 
 * @Param:  p_frame: Puntero a la trama del GPS a analizar
 */
bool SIM808_validateNMEAFrame(uint8_t *p_frame) {
    uint8_t* index = 0;

    index = strstr(p_frame, ATRES_GPS_INFO);
    if (index == NULL) {
        return false;
    }
    //  voy hasta el final de +CGNSINF: 
    index += strlen(ATRES_GPS_INFO) + 1;

    return ( *(index + 2) == '1');
}

bool get_NMEAFrame(uint8_t *buffer) {
    bool result = false;
    if (xSemaphoreTake(c_semGPSIsReady, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (SIM808_getNMEA(buffer)) {
            if (SIM808_validateNMEAFrame(buffer)){
                result = true; 
            }
        }
        xSemaphoreGive(c_semGPSIsReady);
    }
    return result;
}
