#include <stdint.h>
#include <stdbool.h>
#include "drivers.h"

uint32_t configuraSysClock(void) {
  return SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                        SYSCTL_OSC_MAIN |
                        SYSCTL_USE_PLL |
                        SYSCTL_CFG_VCO_480),
                        12000000);
}
void ConfigureUART(uint32_t ui32SysClock)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    //
    // Configure GPIO Pins for UART mode.
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, ui32SysClock);
}

void configuraSinalEntrada(void) {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL); // Habilita GPIO L (entrada PL4 gerador de funcoes)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)); // Aguarda final da habilitação
  
  GPIOPinTypeGPIOInput(GPIO_PORTL_BASE, GPIO_PIN_4); // PL4 como entrada 
  GPIOPadConfigSet(GPIO_PORTL_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);  
}

void configuraBotaoEscala(void) {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
    
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0); // push-button SW1 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

int botaoEscalaApertado(void) {
  return !GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_0);
}

void habilitaPerifericoTimer0(void) {
  SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
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