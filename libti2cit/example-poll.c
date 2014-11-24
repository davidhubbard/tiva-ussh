#include <stdbool.h>
#include <stdint.h>

#include "example-main.h"
#include "libti2cit.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/rom.h"

/*
eep read 00 r81 r08 rf9
eep read 01 r81 r30 r63*
eep read 02 r81 r0b r69[*]
eep read 03 r81 r00 r06
eep read 04 r81 r00 r50
eep read 05 r81 r00 r00
eep read 06 r81 r0c r1a[*]
eep read 07 r81 r04 r1a*
eep read 08 r81 r09 re4*
eep read 09 r81 rb3 rd3*
eep read 0a r81 r03 rd4*
eep read 0b r81 re1 r7f*
eep read 0c r81 r7d r66*
eep read 0d r81 ref r97*
eep read 0e r81 r20 r00
eep read 0f r81 r00 r00
eep read 10 r81 r21 r7f*
eep read 11 r81 r8d r92[*]
eep read 12 r81 rfc r76*
eep read 13 r81 r2b reb*
eep read 14 r81 rf9 re8*
eep read 15 r81 rf8 r7a*
eep read 16 r81 r3f rff - temp alarm high on (customizable)
eep read 17 r81 r00 r00 - temp alarm low on (customizable)
eep read 18 r81 r3f rff - rh% alarm high on (customizable)
eep read 19 r81 r3f rff - rh% alarm high off (customizable)
eep read 1a r81 r00 r00 - rh% alarm low on (customizable)
eep read 1b r81 r00 r00 - rh% alarm low off (customizable)
eep read 1c r81 r00 r27 - 0x7ff can be custom
eep read 1d r81 r00 r00 - "reserved" customizable
eep read 1e r81 r00 r00 - custom ID 1
eep read 1f r81 r00 r00 - custom ID 2

total customizable eeprom: 19 bytes
*/

static uint8_t hih_command_mode_retry(uint32_t sysclock, uint32_t addr, char * str)
{
	static const uint8_t hih_cmd_mode[] = { 0xa0, 0, 0 };
	uint8_t r = libti2cit_m_sync_send(I2C2_BASE, addr << 1, sizeof(hih_cmd_mode)/sizeof(hih_cmd_mode[0]), hih_cmd_mode);
	if (r) {
		UARTsend("cmdmode nack ");
		u8tohex(str, r);
		UARTsend(str);
		UARTsend("\r\n");
		return 1;
	}

	uint8_t hih_eep_read[3] = { 0, 0, 0 };
	uint8_t hih_eep_data[3] = { 0, 0, 0 };
	uint32_t eep;

#if 0
	hih_eep_read[0] = 0x17 | 0x40;
	hih_eep_read[1] = 0x9a;
	hih_eep_read[2] = 0xcb;
	UARTsend("eep wr ");
	if (libti2cit_m_sync_send(I2C2_BASE, (addr << 1), sizeof(hih_eep_read)/sizeof(hih_eep_read[0]), hih_eep_read)) {
		UARTsend("nack\r\n");
		return 1;
	}

	hih_eep_read[1] = 0;
	hih_eep_read[2] = 0;
	ROM_SysCtlDelay(sysclock/832);	// wait 12ms for response
	if (libti2cit_m_sync_send(I2C2_BASE, (addr << 1) | 1, 0, 0)) {
		UARTsend("status nack\r\n");
		return 1;
	}
	if (libti2cit_m_sync_recv(I2C2_BASE, 1, hih_eep_data)) return 1;

	UARTsend(" r");
	u8tohex(str, hih_eep_data[0]);	// must be 0x81 for success, 0x80 means still busy, 0x82 is illegal operation, all else are device failure
	UARTsend(str);
	UARTsend("\r\n");
#endif

	for (eep = 0; eep < 0x20; eep++) {
		UARTsend("eep read ");
		u8tohex(str, eep);
		UARTsend(str);

		hih_eep_read[0] = eep;
		if (libti2cit_m_sync_send(I2C2_BASE, (addr << 1), sizeof(hih_eep_read)/sizeof(hih_eep_read[0]), hih_eep_read)) {
			UARTsend("eepread nack\r\n");
			return 1;
		}
		ROM_SysCtlDelay(sysclock/10000);	// wait 100us for response
		if (libti2cit_m_sync_send(I2C2_BASE, (addr << 1) | 1, 0, 0)) {
			UARTsend("eepread nack\r\n");
			return 1;
		}
		if (libti2cit_m_sync_recv(I2C2_BASE, sizeof(hih_eep_data)/sizeof(hih_eep_data[0]), hih_eep_data)) return 1;

		uint32_t j;
		for (j = 0; j < 3; j++) {
			UARTsend(" r");
			u8tohex(str, hih_eep_data[j]);
			UARTsend(str);
		}
		UARTsend("\r\n");
	}
	return 0;
}

static void hih_command_mode_exit(uint32_t sysclock, uint32_t addr, char * str)
{
	static const uint8_t hih_cmd_mode[] = { 0x80, 0, 0 };
	uint8_t r = libti2cit_m_sync_send(I2C2_BASE, addr << 1, sizeof(hih_cmd_mode)/sizeof(hih_cmd_mode[0]), hih_cmd_mode);
	if (r) {
		UARTsend("cmdmode_exit nack ");
		u8tohex(str, r);
		UARTsend(str);
		UARTsend("\r\n");
	}

	ROM_SysCtlDelay(sysclock/25);		// wait 40ms for device to exit command mode
}

static uint8_t hih_command_mode(uint32_t sysclock, uint32_t addr, char * str)
{
	ROM_GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2 | GPIO_PIN_3, 0);
	ROM_SysCtlDelay(sysclock/1000);		// wait 1ms for device to power down
	ROM_GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_2 | GPIO_PIN_3, GPIO_PIN_3);
	ROM_SysCtlDelay(sysclock/10000);	// wait 100us for device to power up

	uint32_t n;
	for (n = 0; n < 5; n++) {
		if (!hih_command_mode_retry(sysclock, addr, str)) {
			// command mode success
			hih_command_mode_exit(sysclock, addr, str);
			return 0;
		}
	}
	UARTsend("unable to enter command mode\r\n");
	return 1;
}

void main_poll(uint32_t sysclock)
{
	uint32_t addr;
	for (addr = 1; addr < 128; addr++) {
		if (libti2cit_m_sync_send(I2C2_BASE, addr << 1, 0 /*len*/, 0 /*buf*/)) continue;

		ROM_UARTCharPut(UART0_BASE, '[');
		char str[32];
		u8tohex(str, addr);
		UARTsend(str);
		ROM_UARTCharPut(UART0_BASE, ']');

		uint32_t c, v = 0;
		for (c = 0; c < 10; c++) {
			ROM_SysCtlDelay(sysclock/300);

			uint8_t buf[4];
			if (libti2cit_m_sync_send(I2C2_BASE, (addr << 1) | 1, 0 /*len*/, 0 /*buf*/)) break;
			if (libti2cit_m_sync_recv(I2C2_BASE, sizeof(buf)/sizeof(buf[0]), buf)) break;

			ROM_UARTCharPut(UART0_BASE, ' ');
			uint32_t i;
			for (i = 0; i < sizeof(buf)/sizeof(buf[0]); i++) {
				v <<= 8;
				v |= buf[i];
				u8tohex(str, buf[i]);
				UARTsend(str);
			}

			if (!(v & 0x40000000l)) {
				UARTsend("\r\ntemp=");
				int32_t tc1 = ((v & 0xffff) >> 2)*165;
				tc1 = (tc1*100) >> 14;
				tc1 -= 4000;
				printf_int32(str, tc1);
				str[5] = 0;
				str[4] = str[3];
				str[3] = str[2];
				str[2] = '.';
				UARTsend(str);
				UARTsend(" C ");

				tc1 = ((v & 0xffff) >> 2)*297;
				tc1 = (tc1*100) >> 14;
				tc1 -= 40*900/5-3200;
				printf_int32(str, tc1);
				str[5] = 0;
				str[4] = str[3];
				str[3] = str[2];
				str[2] = '.';
				UARTsend(str);
				UARTsend(" F\r\n");

				hih_command_mode(sysclock, addr, str);
				break;
			} else if (v != 0xfffffffflu) {
				c--;	// except for all ff's, retry indefinitely requesting a read from the device
			}
		}
	}

	// no slave example code in polling mode because i2c slave operation is inherently asynchronous

	UARTsend("done\r\n");
}
