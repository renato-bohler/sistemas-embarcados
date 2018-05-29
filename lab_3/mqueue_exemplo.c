/**
 * Exemplo de como enviar strings por fila de correspondência
 **/
#include <stdbool.h>
#include <stdio.h>
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

void main(void)
{
  osKernelInitialize();

  SystemInit();

  send_thread_id = osThreadCreate(osThread(send_thread), NULL);
  recv_thread_id = osThreadCreate(osThread(recv_thread), NULL);

  mqueue = osMailCreate(osMailQ(mqueue), NULL);

  osKernelStart();
  osDelay(osWaitForever);
}