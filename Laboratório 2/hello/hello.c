#include <stdint.h>
#include <stdbool.h>
#include "drivers.h"

#define FREQ_UM_SEGUNDO 0xB71B00
#define FREQ_UM_MILI    0x2EE0 


uint8_t LED_D1 = 0;
uint8_t LED_D2 = 0;
uint32_t ui32SysClock;
bool shouldSendUART = 0;
uint32_t lastCounterValue = 0;

void SysTick_Handler(void){
    lastCounterValue = VALOR_COUNTER;
    shouldSendUART = true;
    VALOR_COUNTER = 0;
}

void config(void) {
  PinoutSet(false, false);
  // Configura o system clock
  ui32SysClock = configuraSysClock();
  
  // Habilita o periférico Timer 0
  habilitaPerifericoTimer0();

  // Desabilita o Timer 0 A para configuração 
  desabilitaTimer0();
  
  habilitaSysTick();
  frequenciaSysTick(FREQ_UM_SEGUNDO);
  
  configuraSinalEntrada();
  
  configuraBotaoEscala();
  
  // Configura o Timer 0 A para receber o sinal do pino PL4 como clock, edge-count up
  configuraTimer0();
  
  // Habilita o Timer 0 A
  habilitaTimer0();
  
  // Habilita a interrupção do SysTick
  habilitaInterrupcaoSysTick();
  
  ConfigureUART(ui32SysClock);
}

int
main(void)
{
  config();
    
  while(1){
    if(botaoEscalaApertado()){ // Testa estado do push-button SW1
      while(botaoEscalaApertado());
      if(VALOR_FREQ == FREQ_UM_SEGUNDO){
        VALOR_FREQ = FREQ_UM_MILI;
      }else{
        VALOR_FREQ = FREQ_UM_SEGUNDO;
      }
    }
    
    if (shouldSendUART) {
      UARTprintf("Frequencia: %d %s\n", lastCounterValue, VALOR_FREQ == FREQ_UM_SEGUNDO ? "Hz" : "kHz");
      shouldSendUART = false;
    }
  }
}
