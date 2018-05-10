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
#define FREQ_UM_MILI    0x5B8D80        //por enquanto ta em 0.5 seg para visualização, 0x1D4C0 é 1ms
uint8_t LED_D1 = 0;
//uint8_t LED_D2 = 0;

void SysTick_Handler(void){
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, LED_D1);
    LED_D1 ^= GPIO_PIN_1;
} // SysTick_Handler

void main(void){ 
  uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_480),
                                              12000000); // PLL em 12MHz?
  
  HWREG(NVIC_ST_CTRL) = 0x5;
  HWREG(NVIC_ST_RELOAD) = FREQ_UM_SEGUNDO;
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION); // Habilita GPIO N (LED D1 = PN1, LED D2 = PN0)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)); // Aguarda final da habilitação
  
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1); // LEDs D1 e D2 como saída
  GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);
  GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL); // Habilita GPIO L (entrada PL4 gerador de funcoes)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)); // Aguarda final da habilitação
  
  GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_4); // PL4 como entrada 
  GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0); // push-button SW1 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
  
  HWREG(0x4003000C) &= 0xFFFFFFFE; // desabilita timer 0
  HWREG(0x40062420) |= 0x10; //configura pino PL4 para ser controlado por hardware alternativo
  HWREG(0x4006252C) |= 0x00030000; //PL4 recebe T0CCP0
  
  HWREG(0x40030000) &= 0xFFFFFFFC;
  HWREG(0x40030000) |= 0x4;   // define que a configuracao estara determinada por GPTMAMR
  HWREG(0x40030004) &= 0xFFFFFFF8;
  HWREG(0x40030004) |= 0x13; //capture-edge mode, count up
  HWREG(0x4003000C) &= 0xFFFFFFF3; // positive edge counter
  // colocar de onde vai contar
  // nao vamos usar interrupçao
  HWREG(0x4003000C) |= 0x1; // habilita timer 0
  //HWREG(TIMER0_TAV_R); //ler valor do counter
  //HWREG(TIMER0_TAV_R) = 0x0;*/
  
  

  HWREG(NVIC_ST_CTRL) |= 0x2;   //enable interrupt
  
  while(1){
    if(GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0) == 0){ // Testa estado do push-button SW1
      if(HWREG(NVIC_ST_RELOAD) == FREQ_UM_SEGUNDO){
        HWREG(NVIC_ST_RELOAD) = FREQ_UM_MILI;
      }else{
        HWREG(NVIC_ST_RELOAD) = FREQ_UM_SEGUNDO;
      }
    }
    
    /*if(HWREG(TIMER0_TAV_R) == 3){
      GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, LED_D2);
      LED_D2 ^= GPIO_PIN_2;
      HWREG(TIMER0_TAV_R) = 0x0;
    }*/
    
  } // while
} // main