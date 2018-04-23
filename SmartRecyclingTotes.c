/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== uartecho.c ========
 */
#include <ti/drivers/I2C.h>
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
/* TI-RTOS Header files */
#include <ti/drivers/PIN.h>
#include <ti/drivers/UART.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/ADC.h>

/* Example/Board Header files */
#include "Board.h"

#include <stdint.h>

#define TASKSTACKSIZE    1024

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];

/* Global memory storage for a PIN_Config table */
static PIN_State ledPinState;
/* Pin driver handles */
static PIN_Handle buttonPinHandle;
static PIN_Handle ledPinHandle;
/* Global memory storage for a PIN_Config table */
static PIN_State buttonPinState;

uint16_t adcValue0;

Semaphore_Struct sem0Struct;
Semaphore_Handle uartSem;

Semaphore_Struct sem1Struct;
Semaphore_Handle adcSem;

#define Board_initI2C() I2C_init();
static PIN_State buttonPinState;
static PIN_State ledPinState;

int range = 0;
int volume = 0;
int weight = 20;

int secondsHand = 0;
int minutesHand = 0;
int hoursHand   = 0;




/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config ledPinTable[] = {
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};


PIN_Config buttonPinTable[] = {
    Board_BUTTON1    | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

void buttonCallbackFxn(PIN_Handle handle, PIN_Id pinId) {
    uint32_t currVal = 0;

    /* Debounce logic, only toggle if the button is still pushed (low) */
    CPUdelay(8000*50);
    if (!PIN_getInputValue(pinId)) {
        /* Toggle LED based on the button pressed */
       Semaphore_post(uartSem);
       System_printf("button pressed and semaphore_post \n");
       System_flush();
    }
}


Void sensorFxn(UArg arg0, UArg arg1)
{

    ADC_Handle   adc;
    ADC_Params   params;
    int_fast16_t res;


    ADC_Params_init(&params);

    while(1){

    Semaphore_pend(adcSem,BIOS_WAIT_FOREVER);

    adc = ADC_open(Board_ADC0, &params);
/*
    if (adc == NULL) {
        System_abort("Error initializing ADC channel 0\n");
    }
    else {
        System_printf("ADC channel 0 initialized\n");
    }
*/
    /* Blocking mode conversion*/
    res = ADC_convert(adc, &adcValue0);

    if (res == ADC_STATUS_SUCCESS) {
        adcValue0 = adcValue0/6.4;

        range = (100-(100*adcValue0)/19);
        //if statement makes we don't take a value while lid is being opened
        if(range >= 0){
        volume = range;
        }




        System_printf("ADC channel 0 convert result: 0x%u percent\n", range);

    }
    else {
        System_printf("ADC channel 0 convert failed\n");
    }

    ADC_close(adc);

    System_flush();
    }
}


void timer()
{


    secondsHand = secondsHand + 1;
    Semaphore_post(adcSem);

    if(secondsHand % 30 == 0){
        //System_printf("sensor read posted \n");
        //System_flush();

    }

    if (secondsHand % 10 == 0){
        Semaphore_post(uartSem);
    }



    if (secondsHand == 60){

        secondsHand = 0;
        minutesHand = minutesHand + 1;

        if(minutesHand == 60){

            minutesHand = 0;
            hoursHand = hoursHand + 1;

            if(hoursHand == 24){
                hoursHand = 0;


            }
        }

    }

    //System_printf("tock \n");
            //System_flush();
}

Void echoFxn(UArg arg0, UArg arg1)
{
    System_printf("beginning of task \n");
    System_flush();
    char input;
    unsigned int i=0;
    UART_Handle uart;
    UART_Params uartParams;
    const char echoPrompt[] = "0x00";
    uint8_t tx_buffer_config[1];
    uint8_t rx_buffer_config[1];
    uint8_t tx_buffer_data[4];
    uint8_t rx_buffer_id[13];

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_NEWLINE;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate =19200; // baud rate of the interface
    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL) {
        System_abort("Error opening the UART");
    }

    while(1){
    System_printf("before semaphore pend. uartSem is %d \n", &uartSem);
    System_flush();

    Semaphore_pend(uartSem,BIOS_WAIT_FOREVER);

    System_printf("after semaphore pend \n");
    System_flush();

    tx_buffer_data[0]=0x02; // Size of data to be sent in bytes
    tx_buffer_data[1]= volume;// Data begins........ upto the size of size defined
    tx_buffer_data[2]= weight;

    UART_write(uart, &tx_buffer_data, 4);
    System_printf("sending Data \n");
    System_flush();
    System_printf("end of function \n");
    System_flush();

    }
}

/*
 *  ======== main ========
 */
int main(void)
{
    Semaphore_Params sem0Params;
    Semaphore_Params sem1Params;

    PIN_Handle ledPinHandle; // This is already defined earlier
    Task_Params taskParams;

    /* Call board init functions */
    Board_initGeneral();
    Board_initADC();
    Board_initUART();

    /* Construct a Semaphore object to be use as a resource lock, inital count 1 */
    Semaphore_Params_init(&sem0Params);
    Semaphore_construct(&sem0Struct, 0, &sem0Params);

     /* Obtain instance handle */
    uartSem = Semaphore_handle(&sem0Struct);

    /* Construct a Semaphore object to be use as a resource lock, inital count 0 */
    Semaphore_Params_init(&sem1Params);
    Semaphore_construct(&sem1Struct, 0, &sem1Params);

     /* Obtain instance handle */
    adcSem = Semaphore_handle(&sem1Struct);

    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, ledPinTable);
    if(!ledPinHandle) {
        System_abort("Error initializing board LED pins\n");
    }

    PIN_setOutputValue(ledPinHandle, Board_LED1, 1);


    buttonPinHandle = PIN_open(&buttonPinState, buttonPinTable);
    if(!buttonPinHandle) {
        System_abort("Error initializing button pins\n");
    }

    /* Setup callback for button pins */
    if (PIN_registerIntCb(buttonPinHandle, &buttonCallbackFxn) != 0) {
        System_abort("Error registering button callback function");
    }

    /* Start BIOS */
    BIOS_start();

    return (0);
}
