// Adaptado para LPC1343 por Prof. Douglas Renaux - Jan 2014

/*----------------------------------------------------------------------------
 *      RL-ARM - RTX
 *----------------------------------------------------------------------------
 *      Name:    BLINKY.C
 *      Purpose: RTX example program
 *----------------------------------------------------------------------------
 *
 * Copyright (c) 1999-2009 KEIL, 2009-2013 ARM Germany GmbH
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  - Neither the name of ARM  nor the names of its contributors may be used 
 *    to endorse or promote products derived from this software without 
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include "cmsis_os.h"

#include "lpc13xx.h"

#include "uart.h"
#include "i2c.h"
#include "gpio.h"
#include "ssp.h"
#include "oled.h"

#define LED_A      0
#define LED_B      1
#define LED_C      2
#define LED_D      3
#define LED_CLK    7

osThreadId tid_phaseA;                  /* Thread id of thread: phase_a      */
osThreadId tid_phaseB;                  /* Thread id of thread: phase_b      */
osThreadId tid_phaseC;                  /* Thread id of thread: phase_c      */
osThreadId tid_phaseD;                  /* Thread id of thread: phase_d      */
osThreadId tid_clock;                   /* Thread id of thread: clock        */
osThreadId tid_lcd;                     /* Thread id of thread: LCD        */

osMutexId mut_GLCD;                     /* Mutex to controll GLCD access     */


/*----------------------------------------------------------------------------
  switch LED on
 *---------------------------------------------------------------------------*/
void LED_on  (unsigned char led) {
  osMutexWait(mut_GLCD, osWaitForever);
  oled_putChar( 5+led*8, 33, 'O', OLED_COLOR_BLACK, OLED_COLOR_WHITE);
  osMutexRelease(mut_GLCD);
}

/*----------------------------------------------------------------------------
  switch LED off
 *---------------------------------------------------------------------------*/
void LED_off (unsigned char led) {
  osMutexWait(mut_GLCD, osWaitForever);
  oled_putChar(5+led*8, 33, 'X', OLED_COLOR_WHITE,OLED_COLOR_BLACK );
  osMutexRelease(mut_GLCD);
}

/*----------------------------------------------------------------------------
  Function 'signal_func' called from multiple tasks
 *---------------------------------------------------------------------------*/
void signal_func (osThreadId tid) {
  osSignalSet(tid_clock, 0x0100);           /* set signal to clock thread    */
  osDelay(500);                             /* delay 500ms                   */
  osSignalSet(tid_clock, 0x0100);           /* set signal to clock thread    */
  osDelay(500);                             /* delay 500ms                   */
  osSignalSet(tid, 0xFFFF);                 /* set signal to thread 'thread' */
  osDelay(500);                             /* delay 500ms                   */
}

/*----------------------------------------------------------------------------
  Thread 1 'phaseA': Phase A output
 *---------------------------------------------------------------------------*/
void phaseA (void const *argument) {
  for (;;) {
    osSignalWait(0x0001, osWaitForever);    /* wait for an event flag 0x0001 */
    LED_on (LED_A);
    signal_func (tid_phaseB);               /* call common signal function   */
    LED_off(LED_A);
  }
}

/*----------------------------------------------------------------------------
  Thread 2 'phaseB': Phase B output
 *---------------------------------------------------------------------------*/
void phaseB (void const *argument) {
  for (;;) {
    osSignalWait(0x0001, osWaitForever);    /* wait for an event flag 0x0001 */
    LED_on (LED_B);
    signal_func (tid_phaseC);               /* call common signal function   */
    LED_off(LED_B);
  }
}

/*----------------------------------------------------------------------------
  Thread 3 'phaseC': Phase C output
 *---------------------------------------------------------------------------*/
void phaseC (void const *argument) {
  for (;;) {
    osSignalWait(0x0001, osWaitForever);    /* wait for an event flag 0x0001 */
    LED_on (LED_C);
    signal_func (tid_phaseD);               /* call common signal function   */
    LED_off(LED_C);
  }
}

/*----------------------------------------------------------------------------
  Thread 4 'phaseD': Phase D output
 *---------------------------------------------------------------------------*/
void phaseD (void const *argument) {
  for (;;) {
    osSignalWait(0x0001, osWaitForever);    /* wait for an event flag 0x0001 */
    LED_on (LED_D);
    signal_func (tid_phaseA);               /* call common signal function   */
    LED_off(LED_D);
  }
}

/*----------------------------------------------------------------------------
  Thread 5 'clock': Signal Clock
 *---------------------------------------------------------------------------*/
void clock (void const *argument) {
  for (;;) {
    osSignalWait(0x0100, osWaitForever);    /* wait for an event flag 0x0100 */
    LED_on (LED_CLK);
    osDelay(80);                            /* delay 80ms                    */
    LED_off(LED_CLK);
  }
}

/*----------------------------------------------------------------------------
  Thread 6 'lcd': LCD Control task
 *---------------------------------------------------------------------------*/
void lcd (void const *argument) {
    char s[30];
    sprintf(s,"Clock: %d",SystemCoreClock);
    oled_init();
    oled_clearScreen(OLED_COLOR_WHITE);

    oled_putString(1,1,  (uint8_t*)"Demo CMSIS-RTOS", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,9,  (uint8_t*)"LPC1343", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,17,  (uint8_t*)"Versao beta", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_putString(1,25,  (uint8_t*)s, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    
    osDelay(4000); 

  for (;;) {
    osMutexWait(mut_GLCD, osWaitForever);
    oled_putString(1,25, (uint8_t*) "Clock", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    osMutexRelease(mut_GLCD);
    osDelay(4000);                          /* delay 4s                      */
  }
}

osMutexDef(mut_GLCD);

osThreadDef(phaseA, osPriorityNormal, 1, 0);
osThreadDef(phaseB, osPriorityNormal, 1, 0);
osThreadDef(phaseC, osPriorityNormal, 1, 0);
osThreadDef(phaseD, osPriorityNormal, 1, 0);
osThreadDef(clock,  osPriorityNormal, 1, 0);
osThreadDef(lcd,    osPriorityNormal, 1, 0);

/*----------------------------------------------------------------------------
  Main Thread 'main'
 *---------------------------------------------------------------------------*/
int main (void) {
  osKernelInitialize();

  SystemInit();
  SystemCoreClockUpdate();
  SSPInit();

  mut_GLCD = osMutexCreate(osMutex(mut_GLCD));

  tid_phaseA = osThreadCreate(osThread(phaseA), NULL);
  tid_phaseB = osThreadCreate(osThread(phaseB), NULL);
  tid_phaseC = osThreadCreate(osThread(phaseC), NULL);
  tid_phaseD = osThreadCreate(osThread(phaseD), NULL);
  tid_clock  = osThreadCreate(osThread(clock),  NULL);
  tid_lcd    = osThreadCreate(osThread(lcd),    NULL);

  osSignalSet(tid_phaseA, 0x0001);          /* set signal to phaseA thread   */

  osKernelStart();
  osDelay(osWaitForever);
}
