/**
 * Exemplo de como enviar strings por fila de correspondência
 **/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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
#include "cmsis_os.h"
#include "system_tm4c1294ncpdt.h" // CMSIS-Core
#include "driverleds.h"           // device drivers
#define QUEUE_LIMIT 16

osThreadId send_thread_id;
osThreadId recv_thread_id;

void send_thread(void const *argument);
void recv_thread(void const *argument);

osThreadDef(send_thread, osPriorityNormal, 1, 0);
osThreadDef(recv_thread, osPriorityNormal, 1, 0);

osMailQDef(mqueue, QUEUE_LIMIT, char);
osMailQId mqueue;

void send_thread(void const *argument)
{
  char *message = osMailAlloc(mqueue, osWaitForever);
  message = "initialized\n";
  osMailPut(mqueue, message);

  message = osMailAlloc(mqueue, osWaitForever);
  message = "eA\n";
  osMailPut(mqueue, message);

  message = osMailAlloc(mqueue, osWaitForever);
  message = "d09s\n";
  osMailPut(mqueue, message);

  osThreadYield();
}

void recv_thread(void const *argument)
{
  char *message;
  osEvent event;

  while (1)
  {
    event = osMailGet(mqueue, osWaitForever);
    if (event.status == osEventMail)
    {
      message = event.value.p;
      printf("%s", message);
      osMailFree(mqueue, message);
    }
  }
}

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
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        // eA\xDcA
        //
        // Read the next character from the UART and write it back to the UART.
        //
        ROM_UARTCharPutNonBlocking(UART0_BASE,
                                   ROM_UARTCharGetNonBlocking(UART0_BASE));

        //
        // Blink the LED to show a character transfer is occuring.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        //
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        //
        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        //
        // Turn off the LED
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    }
}

//*****************************************************************************
//
// Send a string to the UART.
//
//*****************************************************************************
void
UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        ROM_UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

void main(void)
{
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

  UARTSend((uint8_t *)"cr\xD", 4);
  
  osKernelInitialize();

  SystemInit();

  send_thread_id = osThreadCreate(osThread(send_thread), NULL);
  recv_thread_id = osThreadCreate(osThread(recv_thread), NULL);

  mqueue = osMailCreate(osMailQ(mqueue), NULL);

  osKernelStart();
  osDelay(osWaitForever);
}