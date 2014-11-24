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

typedef struct example_isr_st_ {
	libti2cit_int_st ti2cit;
	uint32_t scan_addr;
	uint32_t scan_found;
	uint32_t talk_count;
	uint32_t sysclock;
} example_isr_st;

example_isr_st i2c2;

static void scan_next_addr(libti2cit_int_st * st, uint32_t status);
static void scan_start()
{
	if (i2c2.scan_addr > 127) return;

	// start by assuming a device is present -- if NACK is received, then it is not present
	i2c2.ti2cit.buf = 0;
	i2c2.ti2cit.user_cb = scan_next_addr;
	i2c2.ti2cit.addr = i2c2.scan_addr << 1;
	i2c2.ti2cit.len = 0;
	i2c2.scan_found = 1;
	libti2cit_m_isr_nofifo_send(&i2c2.ti2cit);
}

static void talk_to_device();
static void scan_next_addr(libti2cit_int_st * st, uint32_t status)
{
	if (status & I2C_MIMR_NACKIM) {
		i2c2.scan_found = 0;	// got a nack for this address
		return;
	}

	// wait for i2c state machine to reach STOP state
	if (!(status & I2C_MIMR_STOPIM)) {
		UARTsend("scan_nxt ");
		i2cInt_isr_dump(status);
		return;
	}

	if (i2c2.scan_found) {
		// ACK received at i2c2.scan_addr: device found
		i2c2.scan_found = 0;
		ROM_UARTCharPut(UART0_BASE, '[');
		char str[8];
		u8tohex(str, i2c2.scan_addr);
		UARTsend(str);
		ROM_UARTCharPut(UART0_BASE, ']');

		i2c2.ti2cit.user_cb = 0;
		i2c2.talk_count = 0;
		talk_to_device();	// talk_to_device() will call scan_next_addr() when it is finished
		return;
	}

	i2c2.scan_addr++;
	scan_start();
}

uint8_t write_buf[] = { 0 };
static void talk_to_device_cb(libti2cit_int_st * st, uint32_t status);
static void talk_to_device()
{
	if (i2c2.talk_count >= 10) {
		// done talking to device, resume scanning
		scan_next_addr(&i2c2.ti2cit, I2C_MIMR_STOPIM);
		return;
	}

	i2c2.ti2cit.buf = write_buf;
	i2c2.ti2cit.user_cb = talk_to_device_cb;
	i2c2.ti2cit.addr = (i2c2.scan_addr << 1) | 1;
	i2c2.ti2cit.len = 0; //sizeof(write_buf);
	libti2cit_m_isr_nofifo_send(&i2c2.ti2cit);
}

uint8_t read_buf[4];
static void talk_to_device_read(libti2cit_int_st * st, uint32_t status);
static void talk_to_device_cb(libti2cit_int_st * st, uint32_t status)
{
	/* TODO: to really be an interrupt example, set up a timer interrupt and run from the timer isr */
	ROM_SysCtlDelay(i2c2.sysclock/300);

	i2c2.ti2cit.buf = read_buf;
	i2c2.ti2cit.user_cb = talk_to_device_read;
	i2c2.ti2cit.len = sizeof(read_buf);
	libti2cit_m_isr_nofifo_recv(&i2c2.ti2cit);
}

static void talk_to_device_read(libti2cit_int_st * st, uint32_t status)
{
	if (status & I2C_MIMR_NACKIM) {
		// retry talk_to_device()
		UARTsend("talk nack\r\n");
		i2c2.ti2cit.user_cb = 0;
		i2c2.talk_count++;
		talk_to_device();
		return;
	}

	if (!(status & I2C_MIMR_STOPIM)) {
		UARTsend("not sure why not in STOP ");
		i2cInt_isr_dump(status);
		return;
	}

	if (i2c2.ti2cit.nread != 4) {
		UARTsend("talk wrong len\r\n");
		i2c2.ti2cit.user_cb = 0;
		i2c2.talk_count++;
		talk_to_device();
		return;
	}

	i2c2.ti2cit.user_cb = 0;
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
		if (v == 0xfffffffflu) i2c2.talk_count++;	// all ff's indicates slave device has crashed
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

	// done talking to device, resume scanning
	scan_next_addr(&i2c2.ti2cit, I2C_MIMR_STOPIM);
}

void main_isrnofifo(uint32_t sysclock)
{
	// initialize to 0
	char * p = (char *) &i2c2;
	uint32_t len = sizeof(i2c2);
	while (len) {
		*(p++) = 0;
		len--;
	}

	i2c2.ti2cit.base = I2C2_BASE;
	i2c2.sysclock = sysclock;

	// initialize I2C2 slave and turn on loopback mode
	uint8_t slave_addr = 0x7f;
	ROM_I2CSlaveInit(i2c2.ti2cit.base, slave_addr);
	// if you enable loopback mode, it just disables the GPIO pins so they no longer carry the i2c signals
	// internally the master and slave are always connected in a "loopback" configuration
	//HWREG(i2c2.ti2cit.base + I2C_O_MCR) |= I2C_MCR_LPBK;

	libti2cit_m_int_clear(&i2c2.ti2cit);
	libti2cit_s_int_clear(&i2c2.ti2cit);

	ROM_IntMasterEnable();
	ROM_IntEnable(INT_I2C2);
	ROM_I2CMasterIntEnableEx(i2c2.ti2cit.base, I2C_MIMR_NACKIM | I2C_MIMR_STOPIM | I2C_MIMR_ARBLOSTIM | I2C_MIMR_CLKIM | I2C_MIMR_IM);
	ROM_I2CSlaveIntEnableEx(i2c2.ti2cit.base, /*I2C_SIMR_STARTIM | I2C_SIMR_STOPIM |*/ I2C_SIMR_DATAIM);

	scan_start();

	uint32_t time_out = 500lu;
	for (; i2c2.scan_addr <= 127 && time_out; ROM_SysCtlDelay(sysclock/10000), time_out--) {
		ROM_UARTCharPut(UART0_BASE, '.');
	}

	if (!time_out) {
		UARTsend("gave up, took too long\r\n");
	} else {
		UARTsend("done\r\n");
	}

	ROM_I2CSlaveDisable(i2c2.ti2cit.base);
	ROM_I2CMasterIntDisable(i2c2.ti2cit.base);
	ROM_I2CSlaveIntDisable(i2c2.ti2cit.base);
	ROM_IntDisable(INT_I2C2);
	ROM_IntMasterDisable();

	libti2cit_m_int_clear(&i2c2.ti2cit);
	libti2cit_s_int_clear(&i2c2.ti2cit);
}

static void i2cInt_simr_dump(uint32_t status)
{
	static char buf[] = "status=0000";
	u16tohex(&buf[7], status);
	buf[11] = 0;
	UARTsend(buf);

	if (status & I2C_SIMR_DATAIM) UARTsend(" RIS");
	if (status & I2C_SIMR_STARTIM) UARTsend(" start");
	if (status & I2C_SIMR_STOPIM) UARTsend(" stop");
	if (status & I2C_SIMR_DMARXIM) UARTsend(" dmaRX");
	if (status & I2C_SIMR_DMATXIM) UARTsend(" dmaTX");
	if (status & I2C_SIMR_TXIM) UARTsend(" tx");
	if (status & I2C_SIMR_RXIM) UARTsend(" rx");
	if (status & I2C_SIMR_TXFEIM) UARTsend(" txFE");
	if (status & I2C_SIMR_RXFFIM) UARTsend(" rxFF");

	UARTsend("\r\n");
}

uint8_t slave_buf[4];
uint32_t slave_buf_pos = 0;
uint8_t master_buf[4];
uint32_t master_buf_pos = 0;
void i2c2Int_isrnofifo()
{
	uint32_t status = libti2cit_m_isr_isr(&i2c2.ti2cit);
	if (status & LIBTI2CIT_ISR_UNEXPECTED) {
		UARTsend("isr_m unexpected ");
		i2cInt_isr_dump(status);
	}
	if (status & I2C_MIMR_ARBLOSTIM) {
		UARTsend("isr_m: arblost\r\n");
	}
	if (status & I2C_MIMR_CLKIM) {
		UARTsend("isr_m: clk timeout\r\n");
	}

	status = libti2cit_s_isr_isr(&i2c2.ti2cit);
	if (status == LIBTI2CIT_ISR_S_START) {
		UARTsend("starting\r\n");
		master_buf_pos = 0;
		slave_buf_pos = 0;
		return;
	}
	if (status == LIBTI2CIT_ISR_S_STOP) {
		UARTsend("stopped\r\n");
		master_buf_pos = 0;
		slave_buf_pos = 0;
		return;
	}
	if (status & LIBTI2CIT_ISR_S_START) {
		UARTsend("ign S_START ");
	}
	if (status & LIBTI2CIT_ISR_S_STOP) {
		UARTsend("ign S_STOP ");
	}
	if (status & I2C_SCSR_QCMDST) {
		master_buf_pos = 0;
		slave_buf_pos = 0;

		if (status & I2C_SCSR_QCMDRW) {
			// master is requesting to read bytes
			UARTsend("s_QUICK read\r\n");
		} else {
			UARTsend("s_QUICK read\r\n");
		}

		// is it possible to NAK a QCMD?
		uint32_t sack = (HWREG(i2c2.ti2cit.base + I2C_O_SACKCTL) | I2C_SACKCTL_ACKOEN) & (~I2C_SACKCTL_ACKOVAL);
		HWREG(i2c2.ti2cit.base + I2C_O_SACKCTL) = sack | I2C_SACKCTL_ACKOVAL;	// send NACK
		HWREG(i2c2.ti2cit.base + I2C_O_SACKCTL) = (sack | I2C_SACKCTL_ACKOVAL) & (~I2C_SACKCTL_ACKOEN);
	} else if (status & I2C_SCSR_QCMDRW) {
		UARTsend("s_QUICK read outside s_QUICK ");
	}

	if (status & I2C_SCSR_RREQ) {
		if (status & I2C_SCSR_FBR) {
			// start of a buf from master
			master_buf_pos = 0;
			slave_buf_pos = 0;
			UARTsend("s_1st ");
		}
		// read the byte in the buf
		uint32_t sack = (HWREG(i2c2.ti2cit.base + I2C_O_SACKCTL) | I2C_SACKCTL_ACKOEN) & (~I2C_SACKCTL_ACKOVAL);
		if (master_buf_pos >= sizeof(master_buf)) {
			(void) HWREG(i2c2.ti2cit.base + I2C_O_SDR);
			HWREG(i2c2.ti2cit.base + I2C_O_SACKCTL) = sack | I2C_SACKCTL_ACKOVAL;	// send NACK
			UARTsend("s_NACK\r\n");
		} else {
			master_buf[master_buf_pos++] = HWREG(i2c2.ti2cit.base + I2C_O_SDR);
			HWREG(i2c2.ti2cit.base + I2C_O_SACKCTL) = sack;	// send ACK
			HWREG(i2c2.ti2cit.base + I2C_O_SACKCTL) = sack & (~I2C_SACKCTL_ACKOEN);
			UARTsend("s_ACK\r\n");
		}
		// signal the non-isr code to look at the buf
	}
	if (status & I2C_SCSR_TREQ) {
		UARTsend("s_TREQ ");
		i2cInt_simr_dump(HWREG(i2c2.ti2cit.base + I2C_O_SIMR));

		/* NOTE: it is a bad idea to do anything more complicated than copying data out of a buffer here
		 * the i2c master can time out if you delay long enough before responding to a _TREQ
		 */
		HWREG(i2c2.ti2cit.base + I2C_O_SDR) = (slave_buf_pos >= sizeof(slave_buf)) ? 0xff : (/*slave_buf[*/slave_buf_pos++);
		// TODO: master can NACK
	}
	if (status & LIBTI2CIT_ISR_UNEXPECTED) {
		UARTsend("isr_s unexpected\r\n");
	}
}
