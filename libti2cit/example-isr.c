#include <stdbool.h>
#include <stdint.h>

#include "example-main.h"
#include "libti2cit.h"

#include "inc/hw_i2c.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/rom.h"

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

static void i2cInt_isr_dump(uint32_t status)
{
	static char buf[] = "status=0000";
	u16tohex(&buf[7], status);
	buf[11] = 0;
	UARTsend(buf);

	if (status & I2C_MIMR_IM) UARTsend(" RIS");
	if (status & I2C_MIMR_CLKIM) UARTsend(" CLK");
	if (status & I2C_MIMR_DMARXIM) UARTsend(" dmaRX");
	if (status & I2C_MIMR_DMATXIM) UARTsend(" dmaTX");
	if (status & I2C_MIMR_NACKIM) UARTsend(" nack");
	if (status & I2C_MIMR_STARTIM) UARTsend(" start");
	if (status & I2C_MIMR_STOPIM) UARTsend(" stop");
	if (status & I2C_MIMR_ARBLOSTIM) UARTsend(" arb");
	if (status & I2C_MIMR_TXIM) UARTsend(" tx");
	if (status & I2C_MIMR_RXIM) UARTsend(" rx");
	if (status & I2C_MIMR_TXFEIM) UARTsend(" txFE");
	if (status & I2C_MIMR_RXFFIM) UARTsend(" rxFF");

	UARTsend("\r\n");
}

uint32_t scan_addr = 0, scan_found = 0, talk_count = 0, g_sysclock = 0;

static void scan_next_addr(uint32_t status);
static void scan_start()
{
	if (scan_addr > 127) return;

	// start by assuming a device is present -- if NACK is received, then it is not present
	scan_found = 1;
	libti2cit_m_isr_nofifo_send(I2C2_BASE, scan_addr << 1, 0 /*len*/, 0 /*buf*/, scan_next_addr);
}

static void talk_to_device();
static void scan_next_addr(uint32_t status)
{
	if (status & I2C_MIMR_NACKIM) {
		scan_found = 0;	// got a nack for this address
		return;
	}

	// wait for i2c state machine to reach STOP state
	if (!(status & I2C_MIMR_STOPIM)) {
		UARTsend("scan_nxt ");
		i2cInt_isr_dump(status);
		return;
	}

	if (scan_found) {
		scan_found = 0;
		ROM_UARTCharPut(UART0_BASE, '[');
		char str[8];
		u8tohex(str, scan_addr);
		UARTsend(str);
		ROM_UARTCharPut(UART0_BASE, ']');

		// ACK received at this address: device found at scan_addr
		libti2cit_isr_clear();
		talk_count = 0;
		talk_to_device();	// talk_to_device() will call scan_next_addr() when it is finished
		return;
	}

	scan_addr++;
	scan_start();
}

static void talk_to_device_cb(uint32_t status);
static void talk_to_device()
{
	if (talk_count >= 10) {
		// done talking to device, resume scanning
		scan_next_addr(I2C_MIMR_STOPIM);
		return;
	}

	libti2cit_m_isr_nofifo_send(I2C2_BASE, (scan_addr << 1) | 1, 0 /*len*/, 0 /*buf*/, talk_to_device_cb);
}

uint8_t read_buf[4];
static void talk_to_device_read(uint32_t status, uint32_t nread);
static void talk_to_device_cb(uint32_t status)
{
	/* TODO: to really be an interrupt example, set up a timer interrupt and run from the timer isr */
	ROM_SysCtlDelay(g_sysclock/300);

	libti2cit_m_isr_nofifo_recv(I2C2_BASE, 4, read_buf, talk_to_device_read);
}

static void talk_to_device_read(uint32_t status, uint32_t nread)
{
	if (status & I2C_MIMR_NACKIM) {
		// retry talk_to_device()
		UARTsend("talk nack\r\n");
		libti2cit_isr_clear();
		talk_count++;
		talk_to_device();
		return;
	}

	if (!(status & I2C_MIMR_STOPIM)) {
		UARTsend("not sure why not in STOP ");
		i2cInt_isr_dump(status);
		return;
	}

	if (nread != 4) {
		UARTsend("talk wrong len\r\n");
		libti2cit_isr_clear();
		talk_count++;
		talk_to_device();
		return;
	}

	libti2cit_isr_clear();
	ROM_UARTCharPut(UART0_BASE, ' ');
	uint32_t v = 0;
	char str[32];
	uint32_t i;
	for (i = 0; i < sizeof(read_buf)/sizeof(read_buf[0]); i++) {
		v <<= 8;
		v |= read_buf[i];
		u8tohex(str, read_buf[i]);
		UARTsend(str);
	}

	if (v & 0x40000000l) {
		// device not ready yet
		talk_to_device();
		return;
	}

	// decode data
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

	if (0) hih_command_mode(g_sysclock, scan_addr, str);

	// done talking to device, resume scanning
	scan_next_addr(I2C_MIMR_STOPIM);
}

void main_isr(uint32_t sysclock)
{
	g_sysclock = sysclock;

	libti2cit_int_clear(I2C2_BASE);

	ROM_IntMasterEnable();
	ROM_IntEnable(INT_I2C2);
	ROM_I2CMasterIntEnableEx(I2C2_BASE, I2C_MIMR_NACKIM | I2C_MIMR_STOPIM | I2C_MIMR_ARBLOSTIM | I2C_MIMR_IM);

	scan_addr = 0;
	scan_found = 0;
	scan_start();

	uint32_t time_out = 1000000000lu;
	for (; scan_addr <= 127 && time_out; ROM_SysCtlDelay(sysclock/50), time_out--) {
		ROM_UARTCharPut(UART0_BASE, '.');
	}

	if (!time_out) {
		UARTsend("gave up, took too long\r\n");
	} else {
		UARTsend("done\r\n");
	}

	ROM_I2CMasterIntDisable(I2C2_BASE);
	ROM_IntDisable(INT_I2C2);
	ROM_IntMasterDisable();

	libti2cit_int_clear(I2C2_BASE);
}

void i2c2Int_isr()
{
	uint32_t status = libti2cit_m_isr_isr(I2C2_BASE);
	if (status & LIBTI2CIT_ISR_UNEXPECTED) {
		UARTsend("isr unexpected ");
		i2cInt_isr_dump(status);
	}
	if (status & LIBTI2CIT_ISR_MCS_ARBLST) {
		UARTsend("isr: MCS arblst\r\n");
	}
}
