#include "cmsis_os.h"
#include "system_tm4c1294ncpdt.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#define QUEUE_LIMIT 64

typedef struct {
  char* conteudo;
} mensagem;

osThreadId thread1_id;
void thread1(void const *argument);
osThreadDef(thread1, osPriorityNormal, 1, 0);
osMailQDef(mqueue, QUEUE_LIMIT, mensagem);
osMailQId mqueue_id;




//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>UART Echo (uart_echo)</h1>
//!
//! This example application utilizes the UART to echo text.  The first UART
//! (connected to the USB debug virtual serial port on the evaluation board)
//! will be configured in 115,200 baud, 8-n-1 mode.  All characters received on
//! the UART are transmitted back to the UART.
//
//*****************************************************************************

//****************************************************************************
//
// System clock rate in Hz.
//
//****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    uint32_t ui32Status;

    //
    // Get the interrrupt status.
    //
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ui32Status);

    //
    // Loop while there are characters in the receive FIFO.
    //
    if (!ROM_UARTCharsAvail(UART0_BASE)) return;
    int i = 0;
    char msg[11] = "";
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        char letra = ROM_UARTCharGetNonBlocking(UART0_BASE);
        if (letra == '\n') {
          return;
        } else if(letra == '\xD'){
          msg[i] = '\0';
          break;
        } else {
          msg[i] = letra;
          i++;
        }
    }
    mensagem *message;
    message = (mensagem *) osMailAlloc(mqueue_id, 0);
    message->conteudo = msg;
    if (msg == "") {
      printf("Msg vazia\n");
    }
    printf("%s\n", message->conteudo);
    osMailPut(mqueue_id, message);
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
/**void
UARTSend(const char *pui8Buffer)
{
    //
    // Loop while there are more characters to send.
    //
    while(*pui8Buffer != '\0')
    {
        //
        // Write the next character to the UART.
        //
        ROM_UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
    ROM_UARTCharPutNonBlocking(UART0_BASE, '\xD');
}*/



/*void thread1(void const *argument){
  mensagem *message;
  osEvent event;

  while (1)
  {
    printf("em cima do event\n");
    event = osMailGet(mqueue_id, osWaitForever);
    printf("depois do event\n");
    if (event.status == osEventMail)
    {
      printf("if1\n");
      message = (mensagem *)event.value.p;
      printf("%s", message->conteudo);
      osMailFree(mqueue_id, message);
    }
  }
} // thread1*/

void main(void){
  
  g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);
  //
  // Enable the GPIO port that is used for the on-board LED.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

  //
  // Enable the GPIO pins for the LED (PN0).
  //
  ROM_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

  //
  // Enable the peripherals used by this example.
  //
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  //
  // Enable processor interrupts.
  //
  ROM_IntMasterEnable();

  //
  // Set GPIO A0 and A1 as UART pins.
  //
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  //
  // Configure the UART for 115,200, 8-N-1 operation.
  //
  ROM_UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                          (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                           UART_CONFIG_PAR_NONE));

  //
  // Enable the UART interrupt.
  //
  ROM_IntEnable(INT_UART0);
  ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
  //
  // Loop forever echoing data through the UART.
  //
  osKernelInitialize();
  
  SystemInit();
  LEDInit(LED1);
  
  //thread1_id = osThreadCreate(osThread(thread1), NULL);
  mqueue_id = osMailCreate(osMailQ(mqueue), NULL);

  osKernelStart();
  
  //osDelay(osWaitForever);
  mensagem *message;
  osEvent event;

  while (1)
  {
    printf("em cima do event\n");
    event = osMailGet(mqueue_id, osWaitForever);
    printf("depois do event\n");
    if (event.status == osEventMail)
    {
      printf("if1\n");
      message = (mensagem *)event.value.p;
      printf("%s", message->conteudo);
      osMailFree(mqueue_id, message);
    }
  }
} // main