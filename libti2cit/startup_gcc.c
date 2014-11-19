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
extern void i2c2IntHandler();



/*
 * The int_vector_table[] is directly read by the CPU when it handles an interrupt.
 *
 * List your ISR here to get your code executed when that particular interrupt fires.
 */

static uint32_t app_stack[64];                  // The initial stack pointer
__attribute__ ((section(".isr_vector"))) int_vector_t int_vector_table[] =
{
	(int_vector_t)((uint32_t)app_stack + sizeof(app_stack)), // 0: stack pointer
	ResetISR,                               //   1: The reset handler
	NmiSR,                                  //   2: The NMI handler
	FaultISR,                               //   3: The hard fault handler
	IntDefaultHandler,                      //   4: The MPU fault handler
	IntDefaultHandler,                      //   5: The bus fault handler
	IntDefaultHandler,                      //   6: The usage fault handler
	0,                                      //   7: Reserved
	0,                                      //   8: Reserved
	0,                                      //   9: Reserved
	0,                                      //  10: Reserved
	IntDefaultHandler,                      //  11: SVCall handler
	IntDefaultHandler,                      //  12: Debug monitor handler
	0,                                      //  13: Reserved
	IntDefaultHandler,                      //  14: The PendSV handler
	IntDefaultHandler,                      //  15: The SysTick handler
	IntDefaultHandler,                      //  16: GPIO Port A
	IntDefaultHandler,                      //  17: GPIO Port B
	IntDefaultHandler,                      //  18: GPIO Port C
	IntDefaultHandler,                      //  19: GPIO Port D
	IntDefaultHandler,                      //  20: GPIO Port E
	IntDefaultHandler,                      //  21: UART0 Rx and Tx
	IntDefaultHandler,                      //  22: UART1 Rx and Tx
	IntDefaultHandler,                      //  23: SSI0 Rx and Tx
	IntDefaultHandler,                      //  24: I2C0 Master and Slave
	IntDefaultHandler,                      //  25: PWM Fault
	IntDefaultHandler,                      //  26: PWM Generator 0
	IntDefaultHandler,                      //  27: PWM Generator 1
	IntDefaultHandler,                      //  28: PWM Generator 2
	IntDefaultHandler,                      //  29: Quadrature Encoder 0
	IntDefaultHandler,                      //  30: ADC Sequence 0
	IntDefaultHandler,                      //  31: ADC Sequence 1
	IntDefaultHandler,                      //  32: ADC Sequence 2
	IntDefaultHandler,                      //  33: ADC Sequence 3
	IntDefaultHandler,                      //  34: Watchdog timer
	IntDefaultHandler,                      //  35: Timer 0 subtimer A
	IntDefaultHandler,                      //  36: Timer 0 subtimer B
	IntDefaultHandler,                      //  37: Timer 1 subtimer A
	IntDefaultHandler,                      //  38: Timer 1 subtimer B
	IntDefaultHandler,                      //  39: Timer 2 subtimer A
	IntDefaultHandler,                      //  40: Timer 2 subtimer B
	IntDefaultHandler,                      //  41: Analog Comparator 0
	IntDefaultHandler,                      //  42: Analog Comparator 1
	IntDefaultHandler,                      //  43: Analog Comparator 2
	IntDefaultHandler,                      //  44: System Control (PLL, OSC, BO)
	IntDefaultHandler,                      //  45: FLASH Control
	IntDefaultHandler,                      //  46: GPIO Port F
	IntDefaultHandler,                      //  47: GPIO Port G
	IntDefaultHandler,                      //  48: GPIO Port H
	IntDefaultHandler,                      //  49: UART2 Rx and Tx
	IntDefaultHandler,                      //  50: SSI1 Rx and Tx
	IntDefaultHandler,                      //  51: Timer 3 subtimer A
	IntDefaultHandler,                      //  52: Timer 3 subtimer B
	IntDefaultHandler,                      //  53: I2C1 Master and Slave
	IntDefaultHandler,                      //  54: CAN0 (TM4C123 Quadrature Encoder 1)
	IntDefaultHandler,                      //  55: CAN1 (TM4C123 CAN0)
	IntDefaultHandler,                      //  56: Ethernet (TM4C123 CAN1)
	IntDefaultHandler,                      //  57: HIB
	IntDefaultHandler,                      //  58: USB MAC
	IntDefaultHandler,                      //  59: PWM Generator 3 (TM4C123 Hibernate)
	IntDefaultHandler,                      //  60: uDMA 0 Software (TM4C123 USB0)
	IntDefaultHandler,                      //  61: uDMA 0 Error (TM4C123 PWM Generator 3)
	IntDefaultHandler,                      //  62: ADC1 Seq0 (TM4C123 uDMA Software Transfer)
	IntDefaultHandler,                      //  63: ADC1 Seq1 (TM4C123 uDMA Error)
	IntDefaultHandler,                      //  64: ADC1 Seq2 (TM4C123 ADC1 Sequence 0)
	IntDefaultHandler,                      //  65: ADC1 Seq3 (TM4C123 ADC1 Sequence 1)
	IntDefaultHandler,                      //  66: EPI 0     (TM4C123 ADC1 Sequence 2)
	IntDefaultHandler,                      //  67: GPIO Port J (TM4C123 ADC1 Sequence 3)
	0,                                      //  68: GPIO Port K
	0,                                      //  69: GPIO Port L
	IntDefaultHandler,                      //  70: SSI 2 (TM4C123 GPIO Port J)
	IntDefaultHandler,                      //  71: SSI 3 (TM4C123 GPIO Port K)
	IntDefaultHandler,                      //  72: UART 3 (TM4C123 GPIO Port L)
	IntDefaultHandler,                      //  73: UART 4 (TM4C123 SSI2 Rx and Tx)
	IntDefaultHandler,                      //  74: UART 5 (TM4C123 SSI3 Rx and Tx)
	IntDefaultHandler,                      //  75: UART 6 (TM4C123 UART3 Rx and Tx)
	IntDefaultHandler,                      //  76: UART 7 (TM4C123 UART4 Rx and Tx)
	i2c2IntHandler,                         //  77: I2C2 (TM4C123 UART5 Rx and Tx)
	IntDefaultHandler,                      //  78: I2C3 (TM4C123 UART6 Rx and Tx)
	IntDefaultHandler,                      //  79: Timer 4A (TM4C123 UART7 Rx and Tx)
	0,                                      //  80: Timer 4B
	0,                                      //  81: Timer 5A
	0,                                      //  82: Timer 5B
	0,                                      //  83: Floating-Point Exception "imprecise"
	IntDefaultHandler,                      //  84: (TM4C123 I2C2 Master and Slave)
	IntDefaultHandler,                      //  85: (TM4C123 I2C3 Master and Slave)
	IntDefaultHandler,                      //  86: I2C4 (TM4C123 Timer 4 subtimer A)
	IntDefaultHandler,                      //  87: I2C5 (TM4C123 Timer 4 subtimer B)
	0,                                      //  88: GPIO Port M
	0,                                      //  89: GPIO Port N
	0,                                      //  90:
	0,                                      //  91: Tamper
	0,                                      //  92: GPIO Port P "Summary or P0"
	0,                                      //  93: GPIO Port P1
	0,                                      //  94: GPIO Port P2
	0,                                      //  95: GPIO Port P3
	0,                                      //  96: GPIO Port P4
	0,                                      //  97: GPIO Port P5
	0,                                      //  98: GPIO Port P6
	0,                                      //  99: GPIO Port P7
	0,                                      // 100: GPIO Port Q "Summary or Q0"
	0,                                      // 101: GPIO Port Q1
	0,                                      // 102: GPIO Port Q2
	0,                                      // 103: GPIO Port Q3
	0,                                      // 104: GPIO Port Q4
	0,                                      // 105: GPIO Port Q5
	0,                                      // 106: GPIO Port Q6
	0,                                      // 107: GPIO Port Q7
	IntDefaultHandler,                      // 108: GPIO Port R (TM4C123 Timer 5 subtimer A)
	IntDefaultHandler,                      // 109: GPIO Port S (TM4C123 Timer 5 subtimer B)
	IntDefaultHandler,                      // 110: SHA/MD5 (TM4C123 Wide Timer 0 subtimer A)
	IntDefaultHandler,                      // 111: AES (TM4C123 Wide Timer 0 subtimer B)
	IntDefaultHandler,                      // 112: DES (TM4C123 Wide Timer 1 subtimer A)
	IntDefaultHandler,                      // 113: LCD (TM4C123 Wide Timer 1 subtimer B)
	IntDefaultHandler,                      // 114: 16/32-Bit Timer 6A (TM4C123 Wide Timer 2 subtimer A)
	IntDefaultHandler,                      // 115: 16/32-Bit Timer 6B (TM4C123 Wide Timer 2 subtimer B)
	IntDefaultHandler,                      // 116: 16/32-Bit Timer 7A (TM4C123 Wide Timer 3 subtimer A)
	IntDefaultHandler,                      // 117: 16/32-Bit Timer 7B (TM4C123 Wide Timer 3 subtimer B)
	IntDefaultHandler,                      // 118: I2C6 (TM4C123 Wide Timer 4 subtimer A)
	IntDefaultHandler,                      // 119: I2C7 (TM4C123 Wide Timer 4 subtimer B)
	IntDefaultHandler,                      // 120: (TM4C123 Wide Timer 5 subtimer A)
	IntDefaultHandler,                      // 121: 1-Wire (TM4C123 Wide Timer 5 subtimer B)
	IntDefaultHandler,                      // 122: (TM4C123 FPU)
	IntDefaultHandler,                      // 123: (TM4C123 PECI 0)
	IntDefaultHandler,                      // 124: (TM4C123 LPC 0)
	IntDefaultHandler,                      // 125: I2C8 (TM4C123 I2C4 Master and Slave)
	IntDefaultHandler,                      // 126: I2C9 (TM4C123 I2C5 Master and Slave)
	IntDefaultHandler,                      // 127: GPIO Port T (TM4C123 GPIO Port M)
	IntDefaultHandler,                      // 128: (TM4C123 GPIO Port N)
#if 0
/*These are only defined for the TM4C123*/

	IntDefaultHandler,                      // 129: (TM4C123 Quadrature Encoder 2)
	IntDefaultHandler,                      // 130: (TM4C123 Fan 0)
	0,                                      // 131:
	IntDefaultHandler,                      // 132: (TM4C123 GPIO Port P (Summary or P0))
	IntDefaultHandler,                      // 133: (TM4C123 GPIO Port P1)
	IntDefaultHandler,                      // 134: (TM4C123 GPIO Port P2)
	IntDefaultHandler,                      // 135: (TM4C123 GPIO Port P3)
	IntDefaultHandler,                      // 136: (TM4C123 GPIO Port P4)
	IntDefaultHandler,                      // 137: (TM4C123 GPIO Port P5)
	IntDefaultHandler,                      // 138: (TM4C123 GPIO Port P6)
	IntDefaultHandler,                      // 139: (TM4C123 GPIO Port P7)
	IntDefaultHandler,                      // 140: (TM4C123 GPIO Port Q (Summary or Q0))
	IntDefaultHandler,                      // 141: (TM4C123 GPIO Port Q1)
	IntDefaultHandler,                      // 142: (TM4C123 GPIO Port Q2)
	IntDefaultHandler,                      // 143: (TM4C123 GPIO Port Q3)
	IntDefaultHandler,                      // 144: (TM4C123 GPIO Port Q4)
	IntDefaultHandler,                      // 145: (TM4C123 GPIO Port Q5)
	IntDefaultHandler,                      // 146: (TM4C123 GPIO Port Q6)
	IntDefaultHandler,                      // 147: (TM4C123 GPIO Port Q7)
	IntDefaultHandler,                      // 148: (TM4C123 GPIO Port R)
	IntDefaultHandler,                      // 149: (TM4C123 GPIO Port S)
	IntDefaultHandler,                      // 150: (TM4C123 PWM 1 Generator 0)
	IntDefaultHandler,                      // 151: (TM4C123 PWM 1 Generator 1)
	IntDefaultHandler,                      // 152: (TM4C123 PWM 1 Generator 2)
	IntDefaultHandler,                      // 153: (TM4C123 PWM 1 Generator 3)
	IntDefaultHandler                       // 154: (TM4C123 PWM 1 Fault)
#endif
};

extern uint32_t _etext;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;

extern int main();

void ResetISR(void)
{
	// 167: copy _data from flash to ram
	uint32_t *src = &_etext;
	uint32_t *dst;
	for (dst = &_data; dst < &_edata; ) *dst++ = *src++;

	// 172: the compiler demands the bss section must be zero after each reset
	__asm("		ldr     r0, =_bss"
	 "\n		ldr     r1, =_ebss"
	 "\n		mov     r2, #0"
	 "\n		.thumb_func"
	 "\n	zero_loop:"
	 "\n		cmp     r0, r1"
	 "\n		it      lt"
	 "\n		strlt   r2, [r0], #4"
	 "\n		blt     zero_loop");

	// 183: enable fpu before calling main() -- note that this is the proper
	// 184: place to change any fpu configs
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
