#include "inc/tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "drivers/pinout.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"

#define REG_GPTMCTL     0x4003000C      // Timer 0 Control
#define REG_GPTMCFG     0x40030000      // Timer 0 Config
#define REG_GPTMTAMR    0x40030004      // Timer 0 Timer A mode
#define REG_GPTMTAV     0x40030050      // Timer 0 Timer A value

#define VALOR_COUNTER   HWREG(REG_GPTMTAV)
#define VALOR_FREQ      HWREG(NVIC_ST_RELOAD)

uint32_t configuraSysClock(void);
void ConfigureUART(uint32_t ui32SysClock);
void configuraSinalEntrada(void);
void configuraBotaoEscala(void);
int botaoEscalaApertado(void);
void habilitaPerifericoTimer0(void);
void desabilitaTimer0(void);
void habilitaTimer0(void);
void configuraTimer0(void);
void habilitaSysTick(void);
void habilitaInterrupcaoSysTick(void);
void frequenciaSysTick(uint32_t freq);