#include "cmsis_os.h"
#include "system_tm4c1294ncpdt.h"
#include "driverleds.h"
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
#define MAX_MSG_SIZE 16

// System clock
uint32_t g_ui32SysClock;

// Struct da mensagem a ser trocada entre as tarefas
typedef struct
{
  char conteudo[MAX_MSG_SIZE];
} mensagem;

/***************************** THREAD DEFINITIONS *****************************/

// Broker
osThreadId thread_broker_id;
void broker();
osThreadDef(broker, osPriorityAboveNormal, 1, 0);

// Async
osThreadId thread_async_id;
void async();
osThreadDef(async, osPriorityNormal, 1, 0);

// Transmissor
osThreadId thread_transmissor_id;
void transmissor();
osThreadDef(transmissor, osPriorityAboveNormal, 1, 0);

// Elevadores (E, C, D)
osThreadId thread_e_id, thread_c_id, thread_d_id;
void elevador(const void *arg);
osThreadDef(elevador, osPriorityNormal, 3, 0);

/*************************** MAIL QUEUE DEFINITIONS ***************************/

// mqueueBroker
osMailQDef(mqueueBroker, QUEUE_LIMIT, mensagem);
osMailQId mqueueBroker;

// mqueueAsync
osMailQDef(mqueueAsync, QUEUE_LIMIT, mensagem);
osMailQId mqueueAsync;

// mqueueTransmissor
osMailQDef(mqueueTransmissor, QUEUE_LIMIT, mensagem);
osMailQId mqueueTransmissor;

// mqueueE
osMailQDef(mqueueE, QUEUE_LIMIT, mensagem);
osMailQId mqueueE;

// mqueueC
osMailQDef(mqueueC, QUEUE_LIMIT, mensagem);
osMailQId mqueueC;

// mqueueD
osMailQDef(mqueueD, QUEUE_LIMIT, mensagem);
osMailQId mqueueD;

/******************************* UART FUNCTIONS *******************************/

// Handler da interrupção UART
void UARTIntHandler(void)
{
  /* TODO: a mensagem "initialized" vem dividida em duas partes quando não é
   * colocado o debugger: "initiali" e "zed\r\n" */
  uint32_t ui32Status;

  ui32Status = ROM_UARTIntStatus(UART0_BASE, true);
  ROM_UARTIntClear(UART0_BASE, ui32Status);

  // Se não há caracteres a serem lidos, retorne
  if (!ROM_UARTCharsAvail(UART0_BASE))
    return;

  // Se houver caracteres, leia todos e os coloque na fila mqueueBroker
  mensagem *message;
  message = (mensagem *)osMailAlloc(mqueueBroker, 0);

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

// Enviador de comandos para o simulador via serial
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

/*************************** THREAD IMPLEMENTATIONS ***************************/

// Broker
void broker()
{
  mensagem *received_message;
  osEvent event;

  while (1)
  {
    // Espera por uma mensagem da fila mqueueBroker sem timeout
    event = osMailGet(mqueueBroker, osWaitForever);
    if (event.status == osEventMail)
    {
      // Mensagem recebida
      received_message = (mensagem *)event.value.p;

      // A mensagem é "initialized"?
      if (strcmp(received_message->conteudo, "initialized") == 0)
      {
        // Repassa a mensagem para mqueueE, mqueueC e mqueueD
        mensagem *send_message_E, *send_message_C, *send_message_D;

        send_message_E = (mensagem *)osMailAlloc(mqueueE, osWaitForever);
        send_message_C = (mensagem *)osMailAlloc(mqueueC, osWaitForever);
        send_message_D = (mensagem *)osMailAlloc(mqueueD, osWaitForever);

        strcpy(send_message_E->conteudo, received_message->conteudo);
        strcpy(send_message_C->conteudo, received_message->conteudo);
        strcpy(send_message_D->conteudo, received_message->conteudo);

        osMailPut(mqueueE, send_message_E);
        osMailPut(mqueueC, send_message_C);
        osMailPut(mqueueD, send_message_D);

        // Continua o fluxo
        osMailFree(mqueueBroker, received_message);
        continue;
      }

      // O segundo caractere da mensagem é "I"?
      if (received_message->conteudo[1] == 'I')
      {
        // Repassa a mensagem para mqueueAsync e segue o fluxo
        mensagem *send_message;
        send_message = (mensagem *)osMailAlloc(mqueueAsync, osWaitForever);
        strcpy(send_message->conteudo, received_message->conteudo);
        osMailPut(mqueueAsync, send_message);
      }

      // O primeiro caractere da mensagem é "e"?
      // TODO: repassa a mensagem para mqueueE e retorna
      // Não esquecer de dar osMailFree antes de retornar

      // O primeiro caractere da mensagem é "c"?
      // TODO: repassa a mensagem para mqueueC e retorna
      // Não esquecer de dar osMailFree antes de retornar

      // O primeiro caractere da mensagem é "d"?
      // TODO: repassa a mensagem para mqueueD e retorna
      // Não esquecer de dar osMailFree antes de retornar

      osMailFree(mqueueBroker, received_message);
    }
  }
}

// Async
void async()
{
  mensagem *received_message;
  osEvent event;

  while (1)
  {
    // Espera por uma mensagem da fila mqueueAsync sem timeout
    event = osMailGet(mqueueAsync, osWaitForever);

    if (event.status == osEventMail)
    {
      // Mensagem recebida
      received_message = (mensagem *)event.value.p;

      // Substitui o segundo caractere da mensagem por "L"
      mensagem *send_message;
      send_message = (mensagem *)osMailAlloc(mqueueTransmissor, osWaitForever);
      strcpy(send_message->conteudo, received_message->conteudo);
      send_message->conteudo[1] = 'L';

      // Repassa a mensagem para mqueueTransmissor
      osMailPut(mqueueTransmissor, send_message);

      osMailFree(mqueueAsync, received_message);
    }
  }
}

// Elevador
void elevador(void const *arg)
{
  // TODO: remover este exemplo e implementar a thread
  char elevador = (char)arg;
  osMailQId mqueueElevador;
  switch (elevador)
  {
  case 'e':
    mqueueElevador = mqueueE;
    break;
  case 'c':
    mqueueElevador = mqueueC;
    break;
  case 'd':
    mqueueElevador = mqueueD;
    break;
  default:
    break;
  }

  mensagem *received_message;
  osEvent event;

  while (1)
  {
    // Espera por uma mensagem da fila mqueueElevador sem timeout
    event = osMailGet(mqueueElevador, osWaitForever);

    if (event.status == osEventMail)
    {
      // Mensagem recebida
      received_message = (mensagem *)event.value.p;

      // Envia um <elevador>r para fins de teste
      mensagem *send_message;
      send_message = (mensagem *)osMailAlloc(mqueueTransmissor, osWaitForever);
      send_message->conteudo[0] = elevador;
      send_message->conteudo[1] = 'r';

      // Repassa a mensagem para mqueueTransmissor
      osMailPut(mqueueTransmissor, send_message);

      osMailFree(mqueueElevador, received_message);
    }
  }
}

// Transmissor
void transmissor()
{
  mensagem *received_message;
  osEvent event;

  while (1)
  {
    // Espera por uma mensagem da fila mqueueTransmissor sem timeout
    event = osMailGet(mqueueTransmissor, osWaitForever);
    if (event.status == osEventMail)
    {
      // Mensagem recebida
      received_message = (mensagem *)event.value.p;

      // Envia a mensagem pela UART
      enviarComando(received_message->conteudo);

      osMailFree(mqueueTransmissor, received_message);
    }
  }
}

/***************************** INITIALIZE SYSTEM ******************************/

// Main
void main(void)
{
  // Configurações de interrupções, periféricos, UART, etc.
  g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                           SYSCTL_OSC_MAIN |
                                           SYSCTL_USE_PLL |
                                           SYSCTL_CFG_VCO_480),
                                          120000000);

  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

  ROM_IntMasterEnable();

  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

  ROM_UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 115200,
                          (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                           UART_CONFIG_PAR_NONE));

  ROM_IntEnable(INT_UART0);
  ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

  // Configurações dos recursos do CMSIS
  osKernelInitialize();

  // Threads
  // TODO: a última task (elevador D) não está sendo instanciada
  thread_broker_id = osThreadCreate(osThread(broker), NULL);
  thread_async_id = osThreadCreate(osThread(async), NULL);
  thread_transmissor_id = osThreadCreate(osThread(transmissor), NULL);
  thread_e_id = osThreadCreate(osThread(elevador), (void *)'e');
  thread_c_id = osThreadCreate(osThread(elevador), (void *)'c');
  thread_d_id = osThreadCreate(osThread(elevador), (void *)'d');

  // Mail queues
  mqueueBroker = osMailCreate(osMailQ(mqueueBroker), NULL);
  mqueueAsync = osMailCreate(osMailQ(mqueueAsync), NULL);
  mqueueC = osMailCreate(osMailQ(mqueueC), NULL);
  mqueueD = osMailCreate(osMailQ(mqueueD), NULL);
  mqueueE = osMailCreate(osMailQ(mqueueE), NULL);
  mqueueTransmissor = osMailCreate(osMailQ(mqueueTransmissor), NULL);

  osKernelStart();

  osDelay(osWaitForever);
}