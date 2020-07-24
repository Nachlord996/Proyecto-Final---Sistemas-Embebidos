/**************************************************************************************/

Módulos para control y manejo del módulo GSM/GPRS/GPS SIM808

Incluye dos módulos:
* SIM808.c y .h
	-> Contiene funciones y tareas para el encendido, inicialización y configuración del SIM808 para GSM y GPS
	-> Provee una función para el envío de SMS
	-> Provee una función para la obtención de una trama GPS y otra para su validación
* GPS.c y .h
	-> Contiene funciones útiles para el trabajo con coordenadas y tiempo del módulo GPS
	-> Provee una función para crear un link a Google Maps con las coordenadas indicadas.
	
/***************************************************************************************/

Se recomienda utilizar las siguientes tareas en el main de freeRTOS:

xTaskCreate( SIM808_taskCheck, "modemTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+1, NULL );
xTaskCreate( SIM808_initModule, "modemIni", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+2, &modemInitHandle );

Secuencia necesaria para obtener datos del GPS:
1) Obtener una trama
2) Verificar que es válida
3) Obtener posición
4) Obtener fecha y hora