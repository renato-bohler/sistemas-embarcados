# Configurações de registradores para o projeto 2

## Para configurar o sinal de entrada como clock do contador.

### Considerando que utilizaremos PL4 como pino de entrada do sinal que alimentará o contador, cuja base é 0x4006 2000

1. Configurar o GPIOAFSEL (GPIO Alternate Function Select) - 0x4006 2420
	- GPIOAFSEL |= 0x10
		- Configura o pino 4 do Port L ser controlado por hardware alternativo

2. Configurar o GPIOPCTL (GPIO Port Control) - 0x4006 252C
	- GPIOPCTL |= 0x0003 0000
		- PMC4 = 0x3 (T0CCP0)
			- Configura o Pino 4 do Port L para receber o sinal T0CCP0

## Para configurar o contador como modo edge-count, contando para cima em positive edges

### Considerando que vamos utilizar o Timer 0 (A), cuja base é 0x4003 000C

1. Configurar GPTMCTL (General Purpose Timers Control) - 0x4003 000C
	- GPTMCTL &= 0xFFFF FFFE
		- TAEN = 0x0
			- Desabilita o Timer A

2. Configurar GPTMCFG (General Purpose Timers Config) - 0x4003 0000
	- GPTMCFG &= 0xFFFF FFFC
	- GPTMCFG |= 0x4
		- Define que a configuração estará determinada por GPTMTAMR
	
3. Configurar GPTMTAMR (General Purpose Timer A Mode) - 0x4003 0004
	- GPTMTAMR &= 0xFFFF FFF8
	- GPTMTAMR |= 0x13
		- TACMR = 0x0
			- Timer A Capture Mode = Edge-Count mode
		- TAMR = 0x3
			- Timer A Mode = Capture mode
		- TACDIR = 0x1
			- Timer A Count Direction = up

4. Configurar GPTMCTL (General Purpose Timers Control) - 0x4003 000C
	- GPTMCTL &= 0xFFFF FFF3
		- TAEVENT = 0x0
			- Positive edge

5. Não precisamos configurar.

6. Não vamos usar interrupção

7. Configurar GPTMCTL (General Purpose Timers Control) - 0x4003 000C
	- GPTMCTL |= 0x1
		- TAEN = 0x1
			- Habilita o Timer A

8. Não vamos usar interrupção.

9. Ler o valor de GPTMTAV (GPTM Timer A Value) - 0x4003 0050

10. Zerar o valor de GPTMTAV (GPTM Timer A Value) - 0x4003 0050