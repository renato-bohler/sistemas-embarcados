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
#define NUM_ANDARES 16
#define TEMPO_PORTA_ABERTA 2000

typedef enum porta {
  fechada = 1,
  aberta
} porta;

typedef enum estado {
  esperando_inicializacao = 1,
  standby,
  fechando_porta,
  abrindo_porta,
  subindo,
  descendo
} estado;

typedef enum sentido {
  elevador_parado = 1,
  elevador_subindo,
  elevador_descendo
} sentido;

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
osThreadDef(broker, osPriorityHigh, 1, 0);

// Async
osThreadId thread_async_id;
void async();
osThreadDef(async, osPriorityAboveNormal, 1, 0);

// Transmissor
osThreadId thread_transmissor_id;
void transmissor();
osThreadDef(transmissor, osPriorityHigh, 1, 0);

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
 * andarNumeroParaLetra
 * Recebe um inteiro (0...15) e mapeia para a letra do andar correspondente (a...p)
 * Uso: desligar botão interno <elevador>D<a...p>
 */
char andarNumeroParaLetra(int andar) {
  // 97 é o valor decimal do caractere 'a' em ASCII
  return (char) andar + 97;
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

/*
 * ehDestino
 * Recebe o andar e os destinos de um elevador e retorna se ele é um destino
 */
int ehDestino(sentido s, int andar, int *destinos_internos, int *destinos_sobe, int *destinos_desce) {
  if (s == elevador_parado) {
    // Se o elevador estiver parado, então qualquer sentido será considerado destino
    if (destinos_internos[andar] || destinos_sobe[andar] || destinos_desce[andar]) {
      return 1;
    }
  } else if (s == elevador_subindo) {
    // Se o elevador estiver subindo, então:
    //  - os pedidos de subida serão considerados destino
    //  - os pedidos internos serão considerados destino
    //  - Se não houver mais ninguém querendo subir, então:
    //    - o pedido de descida do andar mais alto será considerado destino
    if (destinos_sobe[andar] || destinos_internos[andar]) {
      return 1;
    } else if (destinos_desce[andar]) {
      // Verifica se existe algum andar acima deste que também quer descer
      int i, andar_desce_mais_alto = andar, mais_alguem_quer_subir = 0;
      for (i = andar + 1; i < NUM_ANDARES; i++) {
        if (destinos_desce[i]) {
          andar_desce_mais_alto = i;
        } else if (destinos_sobe[i]) {
          mais_alguem_quer_subir = 1;
        }
      }
      if (!mais_alguem_quer_subir && andar == andar_desce_mais_alto) {
        return 1;
      }
    }
  } else if (s == elevador_descendo) {
    // Se o elevador estiver descendo, então:
    //  - os pedidos de descida serão considerados destino
    //  - os pedidos internos serão considerados destino
    //  - o pedido de subida do andar mais baixo será considerado destino
    if (destinos_desce[andar] || destinos_internos[andar]) {
      return 1;
    } else if (destinos_sobe[andar]) {
      // Verifica se existe algum andar abaixo deste que também quer subir
      int i, andar_sobe_mais_baixo = andar;
      for (i = andar - 1; i >= 0; i--) {
        if (destinos_sobe[i]) {
          andar_sobe_mais_baixo = i;
        }
      }
      if (andar == andar_sobe_mais_baixo) {
        return 1;
      }
    }
  }
  
  // Se nenhum destes critérios foi aceito, então o dado andar não é destino
  return 0;
}

/* 
 * possuiDestino
 * Recebe o sentido e os destinos de um elevador e retorna se ele possui destino
 */
int possuiDestino(sentido s, int *destinos_internos, int *destinos_sobe, int *destinos_desce) {
  // Para cada andar
  int i;
  for (i = 0; i < NUM_ANDARES; i++) {
    // Se este andar for destino, então o elevador possui destino
    if (ehDestino(s, i, destinos_internos, destinos_sobe, destinos_desce)) {
      return 1;
    }
  }

  // Se nenhum andar possui destino, então o elevador não possui destino
  return 0;
}

/* 
 * deveSubir
 * Recebe o sentido, o andar e os destinos de um elevador e retorna se ele deve subir
 */
int deveSubir(sentido s, int andar, int *destinos_internos, int *destinos_sobe, int *destinos_desce) {
  // Se o elevador estiver parado ou já estiver subindo
  if (s == elevador_subindo || s == elevador_parado) {
    // Para cada andar acima do andar atual
    int i;
    for (i = andar + 1; i < NUM_ANDARES; i++) {
      // Se este andar for destino, então deve subir
      if (ehDestino(s, i, destinos_internos, destinos_sobe, destinos_desce)) {
        return 1;
      }
    }
  }

  // Se nenhum andar acima está com o destino definido, então não deve subir
  return 0;
}

/* 
 * deveDescer
 * Recebe o sentido, o andar e os destinos de um elevador e retorna se ele deve descer
 */
int deveDescer(sentido s, int andar, int *destinos_internos, int *destinos_sobe, int *destinos_desce) {
  // Se o elevador estiver parado ou já estiver descendo
  if (s == elevador_descendo || s == elevador_parado) {
    // Para cada andar abaixo do andar atual
    int i;
    for (i = andar - 1; i >= 0; i--) {
      // Se este andar for destino, então deve descer
      if (ehDestino(s, i, destinos_internos, destinos_sobe, destinos_desce)) {
        return 1;
      }
    }
  }

  // Se nenhum andar abaixo está com o destino definido, então não deve descer
  return 0;
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

        send_message_E = (mensagem *)osMailCAlloc(mqueueE, osWaitForever);
        send_message_C = (mensagem *)osMailCAlloc(mqueueC, osWaitForever);
        send_message_D = (mensagem *)osMailCAlloc(mqueueD, osWaitForever);

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
        send_message = (mensagem *)osMailCAlloc(mqueueAsync, osWaitForever);
        strcpy(send_message->conteudo, received_message->conteudo);
        osMailPut(mqueueAsync, send_message);
        
        // Continua a tratar esta mensagem
      }

      // O primeiro caractere da mensagem é "e"?
      if (received_message->conteudo[0] == 'e')
      {
        // Repassa a mensagem para mqueueE
        mensagem *send_message;
        send_message = (mensagem *)osMailCAlloc(mqueueE, osWaitForever);
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
        send_message = (mensagem *)osMailCAlloc(mqueueC, osWaitForever);
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
        send_message = (mensagem *)osMailCAlloc(mqueueD, osWaitForever);
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
      send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
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
  estado estado_elevador = esperando_inicializacao;
  int andar = 0;
  porta estado_porta = fechada;
  sentido sentido_elevador = elevador_parado;
  int destinos_internos[NUM_ANDARES];
  int destinos_sobe[NUM_ANDARES];
  int destinos_desce[NUM_ANDARES];

  // Indicadores do conteúdo da última mensagem
  int chegou_em_andar = 0;
  int initialized = 0;
  
  int i;
  for(i=0; i < NUM_ANDARES; i++)
  {
    destinos_internos[i] = 0;
    destinos_sobe[i] = 0;
    destinos_desce[i] = 0;
  }
  
  while (1)
  {
      /***********************************************************/
     /* 1. Espera por uma mensagem vinda da fila correspondente */
    /***********************************************************/
    
    event = osMailGet(mqueueElevador, osWaitForever);

    if (event.status == osEventMail)
    {
      // Mensagem recebida
      received_message = (mensagem *)event.value.p;
      char *msg_conteudo = received_message->conteudo;

        /******************************************************/
       /* 2. Atualização das variáveis de estado do elevador */
      /******************************************************/
      
      // Inicializou
      if (msg_conteudo[0] == 'i') {
        // Mensagem: initialized
        initialized = 1;
      } else {
        initialized = 0;
      }
      
      // Botões apertados
      if (msg_conteudo[1] == 'I') {
        // Mensagem: <elevador>I<a...p> (botão interno)
        int destino_andar = andarLetraParaNumero(msg_conteudo[2]);
        if (andar != destino_andar || estado_porta == fechada) {
          destinos_internos[destino_andar] = 1;
        } else {
          // Desliga o botão interno do andar correspondente
          mensagem *send_message_desliga_botao;
          send_message_desliga_botao = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
          send_message_desliga_botao->conteudo[0] = elevador;
          send_message_desliga_botao->conteudo[1] = 'D';
          send_message_desliga_botao->conteudo[2] = msg_conteudo[2];
          osMailPut(mqueueTransmissor, send_message_desliga_botao);
        }
      } else if (msg_conteudo[1] == 'E') {
        // Mensagem: <elevador>E<00...15><s|d> (botão externo)
        char dezena = msg_conteudo[2];
        char unidade = msg_conteudo[3];
        int destino_andar = andarStringParaNumero(dezena, unidade);
        
        if (andar != destino_andar || estado_porta == fechada) {
          if (msg_conteudo[4] == 's') {
            // Botão externo de subida foi apertado
            destinos_sobe[destino_andar] = 1;
          } else if (msg_conteudo[4] == 'd') {
            // Botão externo de descida foi apertado
            destinos_desce[destino_andar] = 1;
          }
        }
      }
      
      // Chegou ao andar
      if (msg_conteudo[1] >= '0' && msg_conteudo[1] <= '9') {
        // Mensagem: <elevador><0...15> (elevador chegou ao andar)
        
        // Conversão do andar para integer
        char dezena, unidade;
        if (strlen(msg_conteudo) == 3) {
          dezena = msg_conteudo[1];
          unidade = msg_conteudo[2];
        } else {
          dezena = '0';
          unidade = msg_conteudo[1];
        }
        
        // Atualiza a variável de estado andar
        andar = andarStringParaNumero(dezena, unidade);
        chegou_em_andar = 1;
      } else {
        chegou_em_andar = 0;
      }
      
      // Porta abriu
      if (msg_conteudo[1] == 'A') {
        estado_porta = aberta;
      }
      
      // Porta fechou
      if (msg_conteudo[1] == 'F') {
        estado_porta = fechada;
      }
      
        /****************************************************/
       /* 3. Atualização da máquina de estados do elevador */
      /****************************************************/
      
      // Esperando inicialização
      if (estado_elevador == esperando_inicializacao) {
        if (initialized) {
          // Sistema inicializou
          
          // Reseta o elevador
          mensagem *send_message;
          send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
          send_message->conteudo[0] = elevador;
          send_message->conteudo[1] = 'r';
          
          // Repassa a mensagem para mqueueTransmissor
          osMailPut(mqueueTransmissor, send_message);
          osMailFree(mqueueElevador, received_message);
          
          // Transição para standby
          estado_elevador = standby;
          continue;
        }
      }
      
      // Standby
      if (estado_elevador == standby) {
        sentido_elevador = elevador_parado;

        if (possuiDestino(sentido_elevador, destinos_internos, destinos_sobe, destinos_desce)) {
          // Se possuir destino, fecha a porta
          mensagem *send_message;
          send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
          send_message->conteudo[0] = elevador;
          send_message->conteudo[1] = 'f';
          
          // Repassa a mensagem para mqueueTransmissor
          osMailPut(mqueueTransmissor, send_message);
          osMailFree(mqueueElevador, received_message);
          
          // Transição para fechando porta
          estado_elevador = fechando_porta;
          continue;
        }
      }
      
      // Fechando porta
      if (estado_elevador == fechando_porta) {
        if (estado_porta == fechada) {
          // A porta terminou de fechar
          if (deveSubir(sentido_elevador, andar, destinos_internos, destinos_sobe, destinos_desce)) {
            // Manda o elevador começar a subir
            mensagem *send_message;
            send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message->conteudo[0] = elevador;
            send_message->conteudo[1] = 's';
            
            // Repassa a mensagem para mqueueTransmissor
            osMailPut(mqueueTransmissor, send_message);
            
            // Transição para descendo
            estado_elevador = subindo;
          } else if (deveDescer(sentido_elevador, andar, destinos_internos, destinos_sobe, destinos_desce)) {
            // Manda o elevador começar a descer
            mensagem *send_message;
            send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message->conteudo[0] = elevador;
            send_message->conteudo[1] = 'd';
            
            // Repassa a mensagem para mqueueTransmissor
            osMailPut(mqueueTransmissor, send_message);
            
            // Transição para descendo
            estado_elevador = descendo;
          } else if (deveSubir(elevador_parado, andar, destinos_internos, destinos_sobe, destinos_desce)) {
            // Manda o elevador começar a subir
            mensagem *send_message;
            send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message->conteudo[0] = elevador;
            send_message->conteudo[1] = 's';
            
            // Repassa a mensagem para mqueueTransmissor
            osMailPut(mqueueTransmissor, send_message);
            
            // Transição para descendo
            estado_elevador = descendo;
          } else if (deveDescer(elevador_parado, andar, destinos_internos, destinos_sobe, destinos_desce)) {
            // Manda o elevador começar a descer
            mensagem *send_message;
            send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message->conteudo[0] = elevador;
            send_message->conteudo[1] = 'd';
            
            // Repassa a mensagem para mqueueTransmissor
            osMailPut(mqueueTransmissor, send_message);
            
            // Transição para descendo
            estado_elevador = descendo;
          }

          osMailFree(mqueueElevador, received_message);
          continue;
        }
      }
      
      // Subindo
      if (estado_elevador == subindo) {
        sentido_elevador = elevador_subindo;
        
        if (chegou_em_andar) {
          // O elevador acabou de chegar em algum andar
          
          if(ehDestino(sentido_elevador, andar, destinos_internos, destinos_sobe, destinos_desce)) {
            // Chegou em algum andar que é destino
            
            // Pare o elevador
            mensagem *send_message_pare;
            send_message_pare = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message_pare->conteudo[0] = elevador;
            send_message_pare->conteudo[1] = 'p';
            osMailPut(mqueueTransmissor, send_message_pare);
            
            // Abra a porta do elevador
            mensagem *send_message_abre;
            send_message_abre = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message_abre->conteudo[0] = elevador;
            send_message_abre->conteudo[1] = 'a';
            osMailPut(mqueueTransmissor, send_message_abre);
            
            // Remove o andar dos objetivos
            destinos_internos[andar] = 0;
            destinos_sobe[andar] = 0;
            destinos_desce[andar] = 0;
            
            // Desliga o botão interno do andar correspondente
            mensagem *send_message_desliga_botao;
            send_message_desliga_botao = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message_desliga_botao->conteudo[0] = elevador;
            send_message_desliga_botao->conteudo[1] = 'D';
            send_message_desliga_botao->conteudo[2] = andarNumeroParaLetra(andar);
            osMailPut(mqueueTransmissor, send_message_desliga_botao);
            
            // Transição para abrindo porta
            estado_elevador = abrindo_porta;
          }
          
          osMailFree(mqueueElevador, received_message);
          continue;
        }
      }
      
      // Descendo
      if (estado_elevador == descendo) {
        sentido_elevador = elevador_descendo;
        
        if (chegou_em_andar) {
          // O elevador acabou de chegar em algum andar
        
          if(ehDestino(sentido_elevador, andar, destinos_internos, destinos_sobe, destinos_desce)) {
            // Chegou em algum andar que é destino
            
            // Pare o elevador
            mensagem *send_message_pare;
            send_message_pare = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message_pare->conteudo[0] = elevador;
            send_message_pare->conteudo[1] = 'p';
            osMailPut(mqueueTransmissor, send_message_pare);
            
            // Abra a porta do elevador
            mensagem *send_message_abre;
            send_message_abre = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message_abre->conteudo[0] = elevador;
            send_message_abre->conteudo[1] = 'a';
            osMailPut(mqueueTransmissor, send_message_abre);
            
            // Remove o andar dos objetivos
            destinos_internos[andar] = 0;
            destinos_sobe[andar] = 0;
            destinos_desce[andar] = 0;
            
            // Desliga o botão interno do andar correspondente
            mensagem *send_message_desliga_botao;
            send_message_desliga_botao = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message_desliga_botao->conteudo[0] = elevador;
            send_message_desliga_botao->conteudo[1] = 'D';
            send_message_desliga_botao->conteudo[2] = andarNumeroParaLetra(andar);
            osMailPut(mqueueTransmissor, send_message_desliga_botao);
            
            // Transição para abrindo porta
            estado_elevador = abrindo_porta;
          }
          
          osMailFree(mqueueElevador, received_message);
          continue;
        }
      }
      
      // Abrindo porta
      if (estado_elevador == abrindo_porta) {
        if (estado_porta == aberta) {
          // A porta terminou de abrir

          // Permanece um tempo com a porta aberta
          osDelay(TEMPO_PORTA_ABERTA);
          
          // Verifica se possui destino no sentido atual
          if (possuiDestino(sentido_elevador, destinos_internos, destinos_sobe, destinos_desce)) {
            // Fecha a porta do elevador
            mensagem *send_message;
            send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message->conteudo[0] = elevador;
            send_message->conteudo[1] = 'f';
            osMailPut(mqueueTransmissor, send_message);
            
            // Transição para fechando porta
            estado_elevador = fechando_porta;
          } else if (possuiDestino(elevador_parado, destinos_internos, destinos_sobe, destinos_desce)) {
            // Não possui destino no sentido atual, mas possui no sentido contrário
            sentido_elevador = elevador_parado;
            
            // Fecha a porta do elevador
            mensagem *send_message;
            send_message = (mensagem *)osMailCAlloc(mqueueTransmissor, osWaitForever);
            send_message->conteudo[0] = elevador;
            send_message->conteudo[1] = 'f';
            osMailPut(mqueueTransmissor, send_message);
            
            // Transição para fechando porta
            estado_elevador = fechando_porta;
          } else {
            // Transição para standby
            estado_elevador = standby;
          }
        }
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