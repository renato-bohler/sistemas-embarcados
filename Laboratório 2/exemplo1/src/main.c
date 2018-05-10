#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c1294ncpdt.h" // CMSIS-Core
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h" // driverlib
#include "driverlib/gpio.h"
#include "driverlib/systick.h"

#define FREQ_UM_SEGUNDO 0xB71B00
#define FREQ_UM_MILI    0x5B8D80        //por enquanto ta em 0.5 seg para visualiza��o, 0x1D4C0 � 1ms

#define REG_GPTMCTL     0x4003000C      // Timer 0 Control
#define REG_GPTMCFG     0x40030000      // Timer 0 Config
#define REG_GPTMTAMR    0x40030004      // Timer 0 Timer A mode
#define REG_GPTMTAV     0x40030050      // Timer 0 Timer A value

#define VALOR_COUNTER   HWREG(REG_GPTMTAV)

uint8_t LED_D1 = 0;
uint8_t LED_D2 = 0;

void SysTick_Handler(void){
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, LED_D1);
    LED_D1 ^= GPIO_PIN_1;
}

void desabilitaTimer0(void) {
  HWREG(REG_GPTMCTL) &= 0xFFFFFFFE;
}

void habilitaTimer0(void) {
  HWREG(REG_GPTMCTL) |= 0x1;
}

void configuraTimer0(void) {
  // Configura pino PL4 para ser controlado por hardware alternativo
  HWREG(0x40062420) |= 0x10;
  
  // PL4 recebe T0CCP0
  HWREG(0x4006252C) |= 0x00030000;
  
  // Define que a configuracao estara determinada por GPTMAMR
  HWREG(REG_GPTMCFG) &= 0xFFFFFFFC;
  HWREG(REG_GPTMCFG) |= 0x4;
  
  // Capture-edge mode, count up
  HWREG(REG_GPTMTAMR) &= 0xFFFFFFF8;
  HWREG(REG_GPTMTAMR) |= 0x13;
  
  // Positive edge counter
  HWREG(REG_GPTMCTL) &= 0xFFFFFFF3;
}

void habilitaSysTick(void) {
  HWREG(NVIC_ST_CTRL) = 0x5;
}

void habilitaInterrupcaoSysTick(void) {
  HWREG(NVIC_ST_CTRL) |= 0x2;
}

void frequenciaSysTick(uint32_t freq) {
  HWREG(NVIC_ST_RELOAD) = freq;
}

void config(void) {
  // Configura o system clock
  uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_480),
                                              12000000);
  
  // Habilita o perif�rico Timer 0
  SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

  // Desabilita o Timer 0 A para configura��o 
  desabilitaTimer0();
  
  habilitaSysTick();
  frequenciaSysTick(FREQ_UM_SEGUNDO);
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION); // Habilita GPIO N (LED D1 = PN1, LED D2 = PN0)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)); // Aguarda final da habilita��o
  
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1); // LEDs D1 e D2 como sa�da
  GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
  GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL); // Habilita GPIO L (entrada PL4 gerador de funcoes)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)); // Aguarda final da habilita��o
  
  GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_4); // PL4 como entrada 
  GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilita��o
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0); // push-button SW1 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  
  // Configura o Timer 0 A para receber o sinal do pino PL4 como clock, edge-count up
  configuraTimer0();
  
  // Habilita o Timer 0 A
  habilitaTimer0();
  
  // Habilita a interrup��o do SysTick
  habilitaInterrupcaoSysTick();
}

void main(void){
  config();
 
  while(1){
    if(GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0){ // Testa estado do push-button SW1
      if(HWREG(NVIC_ST_RELOAD) == FREQ_UM_SEGUNDO){
        HWREG(NVIC_ST_RELOAD) = FREQ_UM_MILI;
      }else{
        HWREG(NVIC_ST_RELOAD) = FREQ_UM_SEGUNDO;
      }
    }
    
    if(VALOR_COUNTER >= 3){
      GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, LED_D2);
      LED_D2 ^= GPIO_PIN_0;
      VALOR_COUNTER = 0x0;
    }
    
  } // while
} // main