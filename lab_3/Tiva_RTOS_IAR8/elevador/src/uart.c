#include "types.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include <stdint.h>
#include "driverlib/pin_map.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdbool.h>
#include "inc/hw_memmap.h"

/******************************* UART FUNCTIONS *******************************/

/* 
 * UARTIntHandler - Handler da interrupção UART
 * Recebe as informações da UART e envia mensagens para mqueueBroker
 */
void UARTIntHandler(void)
{
  uint32_t ui32Status;

  ui32Status = ROM_UARTIntStatus(UART0_BASE, true);
  ROM_UARTIntClear(UART0_BASE, ui32Status);

  // Se não há caracteres a serem lidos, retorne
  if (!ROM_UARTCharsAvail(UART0_BASE))
    return;

  // Se houver caracteres, leia todos e os coloque na fila mqueueBroker
  mensagem *message;
  message = (mensagem *)osMailCAlloc(mqueueBroker, 0);

  int i = 0;
  while (ROM_UARTCharsAvail(UART0_BASE))
  {
    char letra = ROM_UARTCharGetNonBlocking(UART0_BASE);
    if (letra == '\n')
    {
      break;
    }
    else if (letra == '\r')
    {
      message->conteudo[i] = '\0';
    }
    else
    {
      message->conteudo[i] = letra;
      i++;
    }
  }

  osMailPut(mqueueBroker, message);
}

/* 
 * enviarComando
 * Envia um dado comando para o simulador
 */
void enviarComando(const char *pui8Buffer)
{
  // Enquanto houver caracteres para enviar
  while (*pui8Buffer != '\0')
  {
    // Envia o caractere
    ROM_UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
  }
  // Termina o comando com um \r
  ROM_UARTCharPutNonBlocking(UART0_BASE, '\r');
}