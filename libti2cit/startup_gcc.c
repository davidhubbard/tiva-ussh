/* Copyright (c) 2014 David Hubbard github.com/davidhubbard
 * Example startup file for tiva-ussh libti2cit example.
 * Licensed under the GNU LGPL v3.
 *
 * The most common reason you would need to edit this file is to add or change an
 * interrupt handler. Declare the ISR just like the other functions defined here.
 */

#include <stdint.h>
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"

/*
 * Interrupt Service Routines (ISR's) must return void and must take no arguments.
 */
typedef void (* const int_vector_t)();


/*
 * 90% of the time you want to declare your ISR extern and then put it in a
 * different .c file.
 */
       void ResetISR();
static void NmiSR();
static void FaultISR();
static void IntDefaultHandler();
extern void UART0IntHandler();



/*
 * The int_vector_table[] is directly read by the CPU when it handles an interrupt.
 *
 * List your ISR here to get your code executed when that particular interrupt fires.
 */

static uint32_t app_stack[64];                  // The initial stack pointer
__attribute__ ((section(".isr_vector"))) int_vector_t int_vector_table[] =
{
	(int_vector_t)((uint32_t)app_stack + sizeof(app_stack)),
	ResetISR,                               // The reset handler
	NmiSR,                                  // The NMI handler
	FaultISR,                               // The hard fault handler
	IntDefaultHandler,                      // The MPU fault handler
	IntDefaultHandler,                      // The bus fault handler
	IntDefaultHandler,                      // The usage fault handler
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	IntDefaultHandler,                      // SVCall handler
	IntDefaultHandler,                      // Debug monitor handler
	0,                                      // Reserved
	IntDefaultHandler,                      // The PendSV handler
	IntDefaultHandler,                      // The SysTick handler
	IntDefaultHandler,                      // GPIO Port A
	IntDefaultHandler,                      // GPIO Port B
	IntDefaultHandler,                      // GPIO Port C
	IntDefaultHandler,                      // GPIO Port D
	IntDefaultHandler,                      // GPIO Port E
	IntDefaultHandler/*UART0IntHandler*/,   // UART0 Rx and Tx
	IntDefaultHandler,                      // UART1 Rx and Tx
	IntDefaultHandler,                      // SSI0 Rx and Tx
	IntDefaultHandler,                      // I2C0 Master and Slave
	IntDefaultHandler,                      // PWM Fault
	IntDefaultHandler,                      // PWM Generator 0
	IntDefaultHandler,                      // PWM Generator 1
	IntDefaultHandler,                      // PWM Generator 2
	IntDefaultHandler,                      // Quadrature Encoder 0
	IntDefaultHandler,                      // ADC Sequence 0
	IntDefaultHandler,                      // ADC Sequence 1
	IntDefaultHandler,                      // ADC Sequence 2
	IntDefaultHandler,                      // ADC Sequence 3
	IntDefaultHandler,                      // Watchdog timer
	IntDefaultHandler,                      // Timer 0 subtimer A
	IntDefaultHandler,                      // Timer 0 subtimer B
	IntDefaultHandler,                      // Timer 1 subtimer A
	IntDefaultHandler,                      // Timer 1 subtimer B
	IntDefaultHandler,                      // Timer 2 subtimer A
	IntDefaultHandler,                      // Timer 2 subtimer B
	IntDefaultHandler,                      // Analog Comparator 0
	IntDefaultHandler,                      // Analog Comparator 1
	IntDefaultHandler,                      // Analog Comparator 2
	IntDefaultHandler,                      // System Control (PLL, OSC, BO)
	IntDefaultHandler,                      // FLASH Control
	IntDefaultHandler,                      // GPIO Port F
	IntDefaultHandler,                      // GPIO Port G
	IntDefaultHandler,                      // GPIO Port H
	IntDefaultHandler,                      // UART2 Rx and Tx
	IntDefaultHandler,                      // SSI1 Rx and Tx
	IntDefaultHandler,                      // Timer 3 subtimer A
	IntDefaultHandler,                      // Timer 3 subtimer B
	IntDefaultHandler,                      // I2C1 Master and Slave
	IntDefaultHandler,                      // Quadrature Encoder 1
	IntDefaultHandler,                      // CAN0
	IntDefaultHandler,                      // CAN1
	IntDefaultHandler,                      // CAN2
	IntDefaultHandler,                      // Ethernet
	IntDefaultHandler,                      // Hibernate
	IntDefaultHandler,                      // USB0
	IntDefaultHandler,                      // PWM Generator 3
	IntDefaultHandler,                      // uDMA Software Transfer
	IntDefaultHandler,                      // uDMA Error
	IntDefaultHandler,                      // ADC1 Sequence 0
	IntDefaultHandler,                      // ADC1 Sequence 1
	IntDefaultHandler,                      // ADC1 Sequence 2
	IntDefaultHandler,                      // ADC1 Sequence 3
	0,                                      // Reserved
	0,                                      // Reserved
	IntDefaultHandler,                      // GPIO Port J
	IntDefaultHandler,                      // GPIO Port K
	IntDefaultHandler,                      // GPIO Port L
	IntDefaultHandler,                      // SSI2 Rx and Tx
	IntDefaultHandler,                      // SSI3 Rx and Tx
	IntDefaultHandler,                      // UART3 Rx and Tx
	IntDefaultHandler,                      // UART4 Rx and Tx
	IntDefaultHandler,                      // UART5 Rx and Tx
	IntDefaultHandler,                      // UART6 Rx and Tx
	IntDefaultHandler,                      // UART7 Rx and Tx
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	IntDefaultHandler,                      // I2C2 Master and Slave
	IntDefaultHandler,                      // I2C3 Master and Slave
	IntDefaultHandler,                      // Timer 4 subtimer A
	IntDefaultHandler,                      // Timer 4 subtimer B
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	0,                                      // Reserved
	IntDefaultHandler,                      // Timer 5 subtimer A
	IntDefaultHandler,                      // Timer 5 subtimer B
	IntDefaultHandler,                      // Wide Timer 0 subtimer A
	IntDefaultHandler,                      // Wide Timer 0 subtimer B
	IntDefaultHandler,                      // Wide Timer 1 subtimer A
	IntDefaultHandler,                      // Wide Timer 1 subtimer B
	IntDefaultHandler,                      // Wide Timer 2 subtimer A
	IntDefaultHandler,                      // Wide Timer 2 subtimer B
	IntDefaultHandler,                      // Wide Timer 3 subtimer A
	IntDefaultHandler,                      // Wide Timer 3 subtimer B
	IntDefaultHandler,                      // Wide Timer 4 subtimer A
	IntDefaultHandler,                      // Wide Timer 4 subtimer B
	IntDefaultHandler,                      // Wide Timer 5 subtimer A
	IntDefaultHandler,                      // Wide Timer 5 subtimer B
	IntDefaultHandler,                      // FPU
	IntDefaultHandler,                      // PECI 0
	IntDefaultHandler,                      // LPC 0
	IntDefaultHandler,                      // I2C4 Master and Slave
	IntDefaultHandler,                      // I2C5 Master and Slave
	IntDefaultHandler,                      // GPIO Port M
	IntDefaultHandler,                      // GPIO Port N
	IntDefaultHandler,                      // Quadrature Encoder 2
	IntDefaultHandler,                      // Fan 0
	0,                                      // Reserved
	IntDefaultHandler,                      // GPIO Port P (Summary or P0)
	IntDefaultHandler,                      // GPIO Port P1
	IntDefaultHandler,                      // GPIO Port P2
	IntDefaultHandler,                      // GPIO Port P3
	IntDefaultHandler,                      // GPIO Port P4
	IntDefaultHandler,                      // GPIO Port P5
	IntDefaultHandler,                      // GPIO Port P6
	IntDefaultHandler,                      // GPIO Port P7
	IntDefaultHandler,                      // GPIO Port Q (Summary or Q0)
	IntDefaultHandler,                      // GPIO Port Q1
	IntDefaultHandler,                      // GPIO Port Q2
	IntDefaultHandler,                      // GPIO Port Q3
	IntDefaultHandler,                      // GPIO Port Q4
	IntDefaultHandler,                      // GPIO Port Q5
	IntDefaultHandler,                      // GPIO Port Q6
	IntDefaultHandler,                      // GPIO Port Q7
	IntDefaultHandler,                      // GPIO Port R
	IntDefaultHandler,                      // GPIO Port S
	IntDefaultHandler,                      // PWM 1 Generator 0
	IntDefaultHandler,                      // PWM 1 Generator 1
	IntDefaultHandler,                      // PWM 1 Generator 2
	IntDefaultHandler,                      // PWM 1 Generator 3
	IntDefaultHandler                       // PWM 1 Fault
};

extern uint32_t _etext;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;

extern int main();

void ResetISR(void)
{
	// copy _data from flash to ram
	uint32_t *src = &_etext;
	uint32_t *dst;
	for (dst = &_data; dst < &_edata; ) *dst++ = *src++;

	// the compiler demands the bss section must be zero after each reset
	__asm("		ldr     r0, =_bss"
	 "\n		ldr     r1, =_ebss"
	 "\n		mov     r2, #0"
	 "\n		.thumb_func"
	 "\n	zero_loop:"
	 "\n		cmp     r0, r1"
	 "\n		it      lt"
	 "\n		strlt   r2, [r0], #4"
	 "\n		blt     zero_loop");

	// enable fpu before calling main() -- note that this is the proper
	// place to change any fpu configs
	HWREG(NVIC_CPAC) = ((HWREG(NVIC_CPAC) &
		 ~(NVIC_CPAC_CP10_M | NVIC_CPAC_CP11_M)) |
		NVIC_CPAC_CP10_FULL | NVIC_CPAC_CP11_FULL);

	main();
}

static void NmiSR()
{
	for (;;) ;
}

static void FaultISR()
{
	for (;;) ;
}

static void IntDefaultHandler()
{
	for (;;) ;
}
