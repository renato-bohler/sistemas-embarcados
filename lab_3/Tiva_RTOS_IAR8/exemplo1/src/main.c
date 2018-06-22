#include "cmsis_os.h"
#include "system_tm4c1294ncpdt.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
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
  char conteudo[16];
} mensagem;

osThreadId thread1_id, thread_broker_id;
void thread1(void const *argument);
void thread_broker ();
osThreadDef(thread1, osPriorityNormal, 1, 0);
osThreadDef(thread_broker, osPriorityAboveNormal, 1, 0);
osMailQDef(mqueue, QUEUE_LIMIT, mensagem);
osMailQId mqueue;




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
  
  osSignalSet(thread_broker_id, 0x1);
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
void
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
}

void thread_broker(){
  osEvent evt = osSignalWait(0x1,osWaitForever);
  if (evt.status == osEventSignal){

      //
      // Loop while there are characters in the receive FIFO.
      //
      if (!ROM_UARTCharsAvail(UART0_BASE)) {
        ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
        return;
      }
      
      mensagem *message;
      message = (mensagem *) osMailAlloc(mqueue, osWaitForever);
        
      int i = 0;
      while(ROM_UARTCharsAvail(UART0_BASE))
      {
          char letra = ROM_UARTCharGetNonBlocking(UART0_BASE);
          if (letra == '\n') {
            break;
          } else if(letra == '\xD'){
            message->conteudo[i] = '\0';
          } else {
            message->conteudo[i] = letra;
            i++;
          }
      }
      
      osMailPut(mqueue, message);
      ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
  }
}


void thread1(void const *argument){
  mensagem *message;
  osEvent event;
  //int pedroni;

  while (1)
  {
    event = osMailGet(mqueue, osWaitForever);
    if (event.status == osEventMail)
    {
      message = (mensagem *)event.value.p;
      /*if (strcmp(message->conteudo, "cIa") == 0) {
        pedroni = 2;
      }*/
      osMailFree(mqueue, message);
    }
  }
} // thread1

void main(void){
  
  g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

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

  osKernelInitialize();
  
  thread1_id = osThreadCreate(osThread(thread1), NULL);
  thread_broker_id = osThreadCreate(osThread(thread_broker), NULL);
  mqueue = osMailCreate(osMailQ(mqueue), NULL);

  osKernelStart();
  
  osDelay(osWaitForever);
} // main