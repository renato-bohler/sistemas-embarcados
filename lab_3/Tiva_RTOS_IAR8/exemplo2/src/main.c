#include <stdbool.h>
#include "cmsis_os.h"
#include "system_tm4c1294ncpdt.h" // CMSIS-Core
#include "driverleds.h" // device drivers

uint32_t states = 0;
osTimerId timer1_id;
void led_func(void const *argument);
osTimerDef(timer1, led_func);

void led_func(void const *argument){
  states ^= (uint32_t)argument;
  LEDWrite(LED1, states);
} // led_func

void main(void){
  osKernelInitialize();
  
  SystemInit();
  LEDInit(LED1);
  
  timer1_id = osTimerCreate(osTimer(timer1), osTimerPeriodic, (void *)LED1);
  osTimerStart(timer1_id, 500);

  osKernelStart();
  osDelay(osWaitForever);
} // main