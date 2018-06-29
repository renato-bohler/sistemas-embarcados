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
#define FECHADA 0
#define ABERTA 1

enum estado {
  esperando_inicializacao = 1,
  standby,
  fechando_porta,
  abrindo_porta,
  subindo,
  descendo
};

enum sentido {
  parado = 1,
  subindo,
  descendo
};

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

/*********************************** UTILS ************************************/

/* 
 * andarLetraParaNumero
 * Recebe um char (a...p) e mapeia para o andar correspondente (0...15)
 * Uso: botão interno <elevador>I<a...p>
 */
int andarLetraParaNumero(char andar) {
  // 97 é o valor decimal do caractere 'a' em ASCII
  return (int) andar - 97;
}

/* 
 * andarStringParaNumero
 * Recebe uma string (00...15) e mapeia para o inteiro correspondente (0...15)
 * Usos: botão externo <elevador>E<00...15><s|d>
 *       chegada a andar <elevador><0...15>
 */
int andarStringParaNumero(char dezena, char unidade) {
  // 48 é o valor decimal do caractere '0' em ASCII
  int dez = (int) dezena - 48;
  int uni = (int) unidade - 48;
  
  return dez * 10 + uni;
}

int deveSubir(sentido s, int andar, int *destinos_internos, int *destinos_sobe, int *destinos_desce) {
  // Se o sentido for de subida e houver destinos acima do andar, continua subindo
  return 1;
}
/*************************** THREAD IMPLEMENTATIONS ***************************/

/* 
 * broker
 * Responsável pelo endereçamento correto das mensagens para as demais filas
 */
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
      if (received_message->conteudo[0] == 'i')
      {
        // Repassa a mensagem para mqueueE, mqueueC e mqueueD
        mensagem *send_message_E, *send_message_C, *send_message_D;

        send_message_E = (mensagem *)osMailAlloc(mqueueE, osWaitForever);
        send_message_C = (mensagem *)osMailAlloc(mqueueC, osWaitForever);
        send_message_D = (mensagem *)osMailAlloc(mqueueD, osWaitForever);

        strcpy(send_message_E->conteudo, "initialized");
        strcpy(send_message_C->conteudo, "initialized");
        strcpy(send_message_D->conteudo, "initialized");

        osMailPut(mqueueE, send_message_E);
        osMailPut(mqueueC, send_message_C);
        osMailPut(mqueueD, send_message_D);

        // Espera próxima mensagem
        osMailFree(mqueueBroker, received_message);
        continue;
      }

      // O segundo caractere da mensagem é "I"?
      if (received_message->conteudo[1] == 'I')
      {
        // Repassa a mensagem para mqueueAsync
        mensagem *send_message;
        send_message = (mensagem *)osMailAlloc(mqueueAsync, osWaitForever);
        strcpy(send_message->conteudo, received_message->conteudo);
        osMailPut(mqueueAsync, send_message);
        
        // Continua a tratar esta mensagem
      }

      // O primeiro caractere da mensagem é "e"?
      if (received_message->conteudo[0] == 'e')
      {
        // Repassa a mensagem para mqueueE
        mensagem *send_message;
        send_message = (mensagem *)osMailAlloc(mqueueE, osWaitForever);
        strcpy(send_message->conteudo, received_message->conteudo);
        osMailPut(mqueueE, send_message);
        
        // Espera próxima mensagem
        osMailFree(mqueueBroker, received_message);
        continue;
      }

      // O primeiro caractere da mensagem é "c"?
      if (received_message->conteudo[0] == 'c')
      {
        // Repassa a mensagem para mqueueC
        mensagem *send_message;
        send_message = (mensagem *)osMailAlloc(mqueueC, osWaitForever);
        strcpy(send_message->conteudo, received_message->conteudo);
        osMailPut(mqueueC, send_message);

        // Espera próxima mensagem
        osMailFree(mqueueBroker, received_message);
        continue;
      }

      // O primeiro caractere da mensagem é "d"?
      if (received_message->conteudo[0] == 'd')
      {
        // Repassa a mensagem para mqueueD
        mensagem *send_message;
        send_message = (mensagem *)osMailAlloc(mqueueD, osWaitForever);
        strcpy(send_message->conteudo, received_message->conteudo);
        osMailPut(mqueueD, send_message);
        
        // Espera próxima mensagem
        osMailFree(mqueueBroker, received_message);
        continue;
      }

      // Mensagem desconhecida
      osMailFree(mqueueBroker, received_message);
    }
  }
}

/* 
 * async
 * Responsável por realizar o acionamento dos indicadores dos botões internos
 */
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

/* 
 * elevador
 * Responsável por realizar o controle dos elevadores
 */
void elevador(void const *arg)
{
  // TODO: implementar corretamente
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
  
  // Variáveis de estado
  int andar = 0;
  int porta = FECHADA;
  int destinos_internos[16];
  int destinos_sobe[16];
  int destinos_desce[16];
  int chegou_em_andar = 0;
  estado estado_elevador = esperando_init;

  int i;
  for(i=0; i < 16; i++)
  {
    dest_interno[i] = 0;
    dest_sobe[i] = 0;
    dest_desce[i] = 0;
  }
  
  while (1)
  {
    // 1. Espera por uma mensagem vinda da fila correspondente
    event = osMailGet(mqueueElevador, osWaitForever);

    if (event.status == osEventMail)
    {
      // Mensagem recebida
      received_message = (mensagem *)event.value.p;

      // 2. Atualização das variáveis de estado do elevador
      
      // Esperando inicialização
      if (estado == esperando_inicializacao && received_message->conteudo[0] == 'i') {
        // Reseta o elevador
        mensagem *send_message;
        send_message = (mensagem *)osMailAlloc(mqueueTransmissor, osWaitForever);
        send_message->conteudo[0] = elevador;
        send_message->conteudo[1] = 'r';
        
        // Repassa a mensagem para mqueueTransmissor
        osMailPut(mqueueTransmissor, send_message);
        osMailFree(mqueueElevador, received_message);
        
        // Transição para standby
        estado = standby;

        continue;
      }
      
      // Standby
      if (estado == standby) {
        if (received_message->conteudo[1] == 'I') {
          // Apertou algum botão interno | <elevador>I<a...p>
          int destino_andar = andarLetraParaNumero(received_message->conteudo[2]);
          destinos_internos[destino_andar] = 1;
        } else if (received_message->conteudo[1] == 'E') {
          // Apertou algum botão externo | <elevador>E<00...15><s|d>
          char dezena = received_message->conteudo[2];
          char unidade = received_message->conteudo[3];
          int destino_andar = andarStringParaNumero(dezena, unidade);
          
          if (received_message->conteudo[4] == 's') {
            // Botão externo de subida foi apertado
            destinos_sobe[destino_andar] = 1;
          } else if (received_message->conteudo[4] == 'd') {
            // Botão externo de descida foi apertado
            destinos_desce[destino_andar] = 1;
          }
        }
        
        if (received_message->conteudo[1] == 'I' || received_message->conteudo[1] == 'E') {
          mensagem *send_message;
          send_message = (mensagem *)osMailAlloc(mqueueTransmissor, osWaitForever);
          send_message->conteudo[0] = elevador;
          send_message->conteudo[1] = 'f';
          
          osMailPut(mqueueTransmissor, send_message);
          osMailFree(mqueueElevador, received_message);
          estado = fechando_porta;
          continue;
        }
      }
      
      // Mensagem: <elevador>F
      // Fechando porta
      if (estado == fechando_porta && received_message->conteudo[1] == 'F') {
        
        // Porta foi fechada, manda o elevador subir
        mensagem *send_message;
        send_message = (mensagem *)osMailAlloc(mqueueTransmissor, osWaitForever);
        send_message->conteudo[0] = elevador;
        send_message->conteudo[1] = 's';
        
        osMailPut(mqueueTransmissor, send_message);
        osMailFree(mqueueElevador, received_message);
        continue;
      }
      
      // Mensagem: <elevador><andar (0...15)>
      if (received_message->conteudo[1] >= '0' && received_message->conteudo[1] <= '9') {
        // Só para fins de teste:
        // Chegou em algum andar, pare e abra a porta.
        
        mensagem *send_message_pare;
        send_message_pare = (mensagem *)osMailAlloc(mqueueTransmissor, osWaitForever);
        send_message_pare->conteudo[0] = elevador;
        send_message_pare->conteudo[1] = 'p';
        osMailPut(mqueueTransmissor, send_message_pare);
        
        mensagem *send_message_abre;
        send_message_abre = (mensagem *)osMailAlloc(mqueueTransmissor, osWaitForever);
        send_message_abre->conteudo[0] = elevador;
        send_message_abre->conteudo[1] = 'a';
        osMailPut(mqueueTransmissor, send_message_abre);
        
        // Conversão do andar em integer
        char dezena, unidade;
        if (strlen(received_message->conteudo) == 3) {
          dezena = received_message->conteudo[1];
          unidade = received_message->conteudo[2];
        } else {
          dezena = '0';
          unidade = received_message->conteudo[1];
        }
        int andar = andarStringParaNumero(dezena, unidade);
        
        osMailFree(mqueueElevador, received_message);
        osDelay(2000);
        continue;
      }
      
      // Mensagem: <elevador>E<andar (00...15)><s|d>
      if (received_message->conteudo[1] == 'E') {
        // Conversão do andar em integer
        char dezena, unidade;
        dezena = received_message->conteudo[2];
        unidade = received_message->conteudo[3];
        
        int andar = andarStringParaNumero(dezena, unidade);
        
        osMailFree(mqueueElevador, received_message);
        continue;
      }

      // Mensagem não utilizada
      osMailFree(mqueueElevador, received_message);
    }
  }
}

/* 
 * transmissor
 * Responsável por realizar o envio de todos os comandos para o simulador
 */
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

/* 
 * main
 * Responsável por configurar periféricos e o sistema operacional
 */
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