/* Copyright (c) 2014 David Hubbard github.com/davidhubbard
 * Licensed under the GNU LGPL v3.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "libti2cit.h"

/* include files from SW-EK-TM4C1294XL-2.1.0.12573.exe
 * set IPATH in Makefile to point to the directory where the tivaware directory can be found
 */
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/i2c.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

const uint8_t lookup_hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
void u8tohex(char * out, uint32_t n)
{
	out[0] = lookup_hex[(n >> 4) & 15]; out[1] = lookup_hex[n & 15]; out[2] = 0;
}

void u16tohex(char * out, uint32_t n)
{
	out[0] = lookup_hex[(n >> 12) & 15];	out[1] = lookup_hex[(n >> 8) & 15];
	out[2] = lookup_hex[(n >> 4) & 15];	out[3] = lookup_hex[n & 15];
}

void printf_int32(char * out, int32_t n)
{
	if (n < 0) {
		*(out++) = '-';
		n = -n;
	}

	*out = 0;
	int32_t f = 1000000000l;
	while (f > n) f /= 10;
	if (!f) {
		*(out++) = '0';
	} else while (f) {
		int32_t v = n / f;
		*(out++) = '0' + v;
		n -= v*f;
		f /= 10;
	}
	*out = 0;
}

void UARTsend(char * str)
{
	while (*str) {
		ROM_UARTCharPut(UART0_BASE, *str++);
	}
}



extern void main_poll(uint32_t sysclock);
extern void main_isrnofifo(uint32_t sysclock);
extern void i2c2Int_isrnofifo();
extern void main_isr(uint32_t sysclock);
extern void i2c2Int_isr();

// select who receives i2c interrupts. Note: The hardware can do this for you in the NVIC, much faster.
uint32_t choice;
void i2c2IntHandler()
{
	switch (choice) {
	case '1': UARTsend("bad i2c int\r\n"); break;
	case '2': i2c2Int_isrnofifo(); break;
	case '3': i2c2Int_isr(); break;
	case '4':
	default: UARTsend("unhandled i2c int\r\n"); break;
	}
}


int main(void) {
	uint32_t sysclock;
	do {
		sysclock = ROM_SysCtlClockFreqSet(SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480, 120*1000*1000);
	} while (!sysclock);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_I2C2)) ;
	while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL)) ;
	while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)) ;
	while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)) ;
	ROM_GPIOPinConfigure(GPIO_PL0_I2C2SDA);
	ROM_GPIOPinConfigure(GPIO_PL1_I2C2SCL);
	ROM_GPIOPinTypeI2C(GPIO_PORTL_BASE, GPIO_PIN_0);
	ROM_GPIOPinTypeI2CSCL(GPIO_PORTL_BASE, GPIO_PIN_1);
	HWREG(GPIO_PORTL_BASE + GPIO_O_PUR) |= GPIO_PIN_0 | GPIO_PIN_1;
	ROM_I2CMasterInitExpClk(I2C2_BASE, sysclock, true /*400kHz*/);
	ROM_I2CMasterGlitchFilterConfigSet(I2C2_BASE, I2C_MASTER_GLITCH_FILTER_DISABLED);

	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_2 | GPIO_PIN_3);
	ROM_GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_3);

	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	ROM_UARTConfigSetExpClk(UART0_BASE, sysclock, 115200,
		UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);

	for (;;) {
		UARTsend("\r\n"
			"Select an example:\r\n"
			"  1. Polling\r\n"
			"  2. Interrupts\r\n"
			"  3. Interrupts+FIFO\r\n"
			"  4. Interrupts+FIFO+uDMA\r\n"
			"\r\n");

		uint32_t bad_key = 0;
		do {
			UARTsend("Choice: ");
			choice = ROM_UARTCharGet(UART0_BASE);
			ROM_UARTCharPut(UART0_BASE, choice);
			UARTsend("\r\n");

			switch (choice) {
			case '1': main_poll(sysclock); break;
			case '2': main_isrnofifo(sysclock); break;
			case '3': main_isr(sysclock); break;
			case '4': break;

			default:
				UARTsend("Unknown key pressed.\r\n");
				bad_key = 1;
				break;
			}
		} while (bad_key);
	}
}
