#include <stdbool.h>
#include <stdint.h>
#include "inc/tm4c1294ncpdt.h" // CMSIS-Core
#include "inc/hw_memmap.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h" // driverlib
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/systick.h"

#define PERIODO 60001
#define DELTA 3000

int32_t LED_D4 = PERIODO;
uint8_t updn = 1;

void SysTick_Handler(void){
    if(!updn){
      if(LED_D4 < PERIODO)
        LED_D4 += DELTA;
      else
        updn = 1;
    } // if
    else{
      if(LED_D4 > 1)
        LED_D4 -= DELTA;
      else
        updn = 0;
    } // else
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, LED_D4);
} // SysTick_Handler

void main(void){
  uint32_t ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                              SYSCTL_USE_PLL |
                                              SYSCTL_CFG_VCO_480),
                                              120000000); // PLL em 120MHz
  
  SysTickEnable();
  SysTickPeriodSet(6000000); // f = 100Hz
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION); // Habilita GPIO N (LED D1 = PN1, LED D2 = PN0)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)); // Aguarda final da habilitação
  
  GPIOPadConfigSet(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  GPIODirModeSet(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_DIR_MODE_OUT); // LEDs D1 e D2 como saída
  GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // Habilita GPIO F (LED D3 = PF4, LED D4 = PF0)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)); // Aguarda final da habilitação
    
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_DIR_MODE_OUT); // LED D3 como saída
  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
  GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_DIR_MODE_HW); // LED D4 como PWM
  GPIOPinConfigure(GPIO_PF0_M0PWM0);

  SysCtlPWMClockSet(PWM_SYSCLK_DIV_64);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0); // Habilita PWM 0
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM0)); // Aguarda final da habilitação
  
  PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC | PWM_GEN_MODE_DBG_RUN);
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PERIODO); // Período = 60000/120000000 (f = 500Hz)
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, PERIODO); // Ciclo de trabalho = 100%
  PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);
  PWMGenEnable(PWM0_BASE, PWM_GEN_0);
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1); // push-buttons SW1 e SW2 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

  SysTickIntEnable();

  while(1);
} // main