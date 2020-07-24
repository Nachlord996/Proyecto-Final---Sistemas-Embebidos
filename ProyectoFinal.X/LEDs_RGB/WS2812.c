/* ************************************************************************** */
/** WS2812

  @Company
    BQN

  @File Name
    WS2812.c

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
#include "WS2812.h"
#include "../mcc_generated_files/pin_manager.h"
#include "../mcc_generated_files/interrupt_manager.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */
ws2812_t const WHITE = {255, 255, 255};
ws2812_t const RED = {255, 0, 0};
ws2812_t const GREEN = {0, 255, 0};
ws2812_t const BLUE = {0, 0, 255};
ws2812_t const BLACK = {0, 0, 0};


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

/* ************************************************************************** */

/** 
  @Function
    static uint32_t WS2812_bitFlip( uint8_t p_b )

  @Summary
    Invierte el orden de los bits en un byte.

  @Description
    Invierte el orden de los bits en un byte.
    <p>
    Se utilizan rotaciones para ser m�s eficiente
    <p>

  @Precondition
    None

  @Parameters
    @param p_b Byte a ser modificado.

  @Returns
    uint32_t Devuelve el valor del byte en 32 bits

  @Remarks
    None
 */

static uint32_t WS2812_bitFlip(uint8_t p_b) {
    p_b = (p_b & 0xF0) >> 4 | (p_b & 0x0F) << 4;
    p_b = (p_b & 0xCC) >> 2 | (p_b & 0x33) << 2;
    p_b = (p_b & 0xAA) >> 1 | (p_b & 0x55) << 1;
    return (uint32_t) p_b;
}

static void WS2812_sendSingle(ws2812_t* p_led) {
    uint8_t j;
    uint32_t val;
    // the WS2812 wants bits in the order of:
    // GGGGGGGGRRRRRRRRBBBBBBBB
    // but I want to work in the opposite order. so i'm going to flip
    // the bits around and do some shifting so my order is
    // BBBBBBBBRRRRRRRRGGGGGGGG
    // with the most significant bit on the far right. so the RGB value
    // of 128 64 32, which normally would be:
    // R : 0b10000000
    // G : 0b01000000
    // B : 0b00100000
    // will become:
    // BBBBBBBBRRRRRRRRGGGGGGGG
    // 000001000000000100000010
    val = (WS2812_bitFlip(p_led->b) << 16) + (WS2812_bitFlip(p_led->r) << 8) + (WS2812_bitFlip(p_led->g));

    // now begin shifting them over one at a time
    for (j = 0; j < 24; j++) {
        // depending on if the currently viewed bit is 1 or 0
        // the pin will stay high for different times
        if (val & 1) {
            // if it is a 1, let it stay higher a bit longer
            LED_CTRL_SetHigh(); // 0.90 us
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            LED_CTRL_SetLow(); // 0.35 us
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
        } else {
            // but a 0 should go high and then low as fast as possible
            /**/
            LED_CTRL_SetHigh(); // 0.35 us
            _nop();
            _nop();
            _nop();
            LED_CTRL_SetLow(); // 0.90 us
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
            _nop();
        }

        // and then right shift to get the next bit
        val = val >> 1;
    }
}

ws2812_t getColor(uint8_t cr) {
    ws2812_t color;
    switch (cr) {
        case 0:
            color = WHITE;
            break;
        case 1:
            color = RED;
            break;
        case 2:
            color = BLUE;
            break;
        case 3:
            color = GREEN;
            break;
        default:
            color = BLACK;
            break;
    }
    return color;
}
/*
int getLEDPosition(uint8_t index){
    int position;
    switch(index){
        case 0:
            position = 0;
            break;
        case 1: /////
            position = 2;
            break;
        case 2:
            position = 3;
            break;
        case 3:
            position = 0;
            break;
        case 4://///
            position = 1;
            break;
        case 5:
            position = 0;
            break;
        case 6:
            position = 0;/////
            break;
        case 7:
            position = 0;
            break;
    }
}
 * */


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

// *****************************************************************************

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
void WS2812_send(ws2812_t *p_leds, uint8_t p_length) {
    uint8_t i;

    INTERRUPT_GlobalDisable();

    //WS2812_sendMultiple( p_leds, p_length );
    for (i = 0; i < p_length; i++) {
        WS2812_sendSingle((p_leds + i));
    }

    INTERRUPT_GlobalEnable();
}

void RGB_LED_eventHandler() {
    WS2812_send(leds, 8);
}

void WS2812_set_Sending_State() {
    char i;
    ws2812_t color;
    if (leds[0].b == 0 & leds[0].r == 0 & leds[0].g == 0){
        color = BLUE;
    } else {
        color = BLACK;
    }
    for (i = 0; i < 8; i++) {
        leds[i] = color;
    }
}

void WS2812_turnOff() {
    char i;
    for (i = 0; i < 8; i++) {
        leds[i] = BLACK;
    }
    RGB_LED_eventHandler();
}
void WS2812_indicateSafeness(bool criticalTemp){
    ws2812_t color;
    char i;
    if (criticalTemp){
        color= RED;
    }
    else{
        color=GREEN;
    }
    for(i=0;i<8;i++){
        leds[i]=color;
    }
}

/* *****************************************************************************
 End of File
 */
