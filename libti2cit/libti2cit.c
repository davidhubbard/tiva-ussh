#include <stdbool.h>
#include <stdint.h>
#include "libti2cit.h"

#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "driverlib/i2c.h"
#include "driverlib/rom.h"


/* wait for I2C_O_MRIS (Raw Interrupt Status)
 * when waiting for a bit to get set, ACK by writing 'mris' to I2C_O_MICR
 */
static uint32_t libti2cit_mris_wait(uint32_t base, uint32_t mask, uint32_t match) {
	uint32_t mris;
	do {
		mris = HWREG(base + I2C_O_MRIS);
	} while ((mris & mask) != match);
	if (match) HWREG(base + I2C_O_MICR) = mris;
	return mris;
}

/* update i2c hardware state machine, then wait for I2C_MRIS_RIS
 */
static uint32_t libti2cit_m_continue(uint32_t base, uint32_t cmd) {
	ROM_I2CMasterControl(base, cmd);
	while (!ROM_I2CMasterBusy(base));
	return libti2cit_mris_wait(base, I2C_MRIS_RIS, I2C_MRIS_RIS) & I2C_MRIS_NACKRIS;
}

/* see description in libti2cit.h
 */
uint8_t libti2cit_m_sync_send(uint32_t base, uint8_t addr, uint8_t len, const uint8_t * buf) {
	uint8_t cmd = I2C_MASTER_CMD_QUICK_COMMAND;
	uint32_t mris_want = I2C_MRIS_RIS | I2C_MRIS_STOPRIS;	// case 1: len == 0 && (addr & 1) == 0

	// this do {} while () is only needed in the case where an i2c repeated start is sent
	do {
		if (len) {
			cmd = I2C_MASTER_CMD_BURST_SEND_START;
			mris_want = I2C_MRIS_RIS;	// case 2: len != 0

			// the tiva i2c hardware wants the first data byte before the i2c start condition is sent
			if (buf) HWREG(base + I2C_O_MDR) = *(buf++); // a.k.a. ROM_I2CMasterDataPut()

		} else if (addr & 1) {
			cmd = I2C_MASTER_CMD_BURST_RECEIVE_START;
			mris_want = I2C_MRIS_RIS;	// case 3: len == 0 && (addr & 1) == 1
		}

		HWREG(base + I2C_O_MSA) = addr;	// a.k.a. ROM_I2CMasterSlaveAddrSet()
		ROM_I2CMasterControl(base, cmd);
		while (!ROM_I2CMasterBusy(base));
		if (libti2cit_mris_wait(base, mris_want, mris_want) & I2C_MRIS_NACKRIS) return 1;
		if (HWREG(base + I2C_O_MCS) & I2C_MCS_ARBLST) return 2;
		if (!len) return 0;
		len--;	// first byte was already sent
		while (len) {
			HWREG(base + I2C_O_MDR) = *(buf++); // a.k.a. ROM_I2CMasterDataPut()
			uint32_t cmd = (--len | (addr & 1)) ? I2C_MASTER_CMD_BURST_SEND_CONT : I2C_MASTER_CMD_BURST_SEND_FINISH;
			if (libti2cit_m_continue(base, cmd)) return 3 + len;
			if (HWREG(base + I2C_O_MCS) & I2C_MCS_ARBLST) return 2;
		}
	} while (addr & 1);	// this will do an i2c repeated start (no i2c stop) and then return from the function

	libti2cit_mris_wait(base, I2C_MRIS_STOPRIS | I2C_MRIS_RIS, 0);
	return 0;
}

/* see description in libti2cit.h
 */
uint8_t libti2cit_m_sync_recv(uint32_t base, uint8_t len, uint8_t * buf) {
	// first byte was already received by i2c state machine
	if (!len) {	// len must be non-zero
		ROM_I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_ERROR_STOP);
		return 1;
	}

	// first byte already received by i2c hardware: read it, then check len
	while (*(buf++) = HWREG(base + I2C_O_MDR) /* a.k.a. ROM_I2CMasterDataGet() */, len) {
		len--;
		libti2cit_m_continue(base, len ? I2C_MASTER_CMD_BURST_RECEIVE_CONT : I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
	}
	libti2cit_mris_wait(base, I2C_MRIS_STOPRIS | I2C_MRIS_RIS, 0);
	return 0;
}




/* see description in libti2cit.h
 */
uint32_t libti2cit_int_clear(uint32_t base) {
	uint32_t status = HWREG(base + I2C_O_MMIS);

	HWREG(base + I2C_O_MICR) = status;
	if (status == I2C_MMIS_MIS) {
		/* Workaround for I2C master interrupt clear errata for rev B Tiva
		 * devices.  For later devices, this write is ignored and therefore
		 * harmless other than the slight performance hit.
		 */
		HWREG(base + I2C_O_MMIS) = I2C_MMIS_MIS;
	}

	return status;
}


















static void (* isr_cb)(uint32_t base, uint32_t status) = 0;
static libti2cit_status_cb isr_user_cb = 0;
static libti2cit_read_cb isr_user_read_cb = 0;
static uint32_t isr_len = 0, isr_nread = 0;
static uint8_t * isr_buf = 0;

static uint32_t libti2cit_m_isr_finish(uint32_t status) {
	if (!isr_user_cb && !isr_user_read_cb) {
		//UARTsend("!isr_user_{read}_cb\r\n");
		return status | LIBTI2CIT_ISR_UNEXPECTED;
	}

	libti2cit_status_cb u = isr_user_cb;
	libti2cit_read_cb ur = isr_user_read_cb;
	if (u) u(status);
	if (ur) ur(status, isr_nread);
	return status;
}

void libti2cit_isr_clear() {
	isr_user_cb = 0;
	isr_user_read_cb = 0;
}

/* see description in libti2cit.h
 */
uint32_t libti2cit_m_isr_isr(uint32_t base) {
	uint32_t status = libti2cit_int_clear(base);

	if (status & I2C_MIMR_NACKIM) {
		if (!isr_user_cb && !isr_user_read_cb) {
			//UARTsend("!isr_user_{read}_cb\r\n");
			status |= LIBTI2CIT_ISR_UNEXPECTED;	// signal UNEXPECTED
		}
	} else {
		// normal isr_cb
		if (isr_cb) {
			isr_cb(base, status);

			if (HWREG(base + I2C_O_MCS) & I2C_MCS_ARBLST) {
				status |= LIBTI2CIT_ISR_MCS_ARBLST;
				//if (HWREG(base + I2C_O_MCS) & I2C_MCS_ARBLST) UARTsend("MCS: arblst\r\n");
			}
			return status;
		}
		// isr_cb was not set...UNEXPECTED
		//UARTsend("!isr_cb\r\n");
		status |= LIBTI2CIT_ISR_UNEXPECTED;
	}
	return libti2cit_m_isr_finish(status);
}

static void libti2cit_m_isr_nofifo_done_ris_stopris(uint32_t base, uint32_t status) {
	// this happens in 2 cases:
	// 1. finished writing 1+ bytes in libti2cit_m_isr_nofifo_send_ris(), now want to stop
	// 2. finished writing 0 bytes, want to stop (came directly from libti2cit_m_isr_nofifo_send())
	if (status & I2C_MIMR_STOPIM) {
		isr_cb = 0;
		libti2cit_m_isr_finish(I2C_MIMR_STOPIM);	// signal all done
	}
}
static void libti2cit_m_isr_nofifo_done_ris(uint32_t base, uint32_t status) {
	// this happens in 2 cases:
	// 1. finished writing 1+ bytes in libti2cit_m_isr_nofifo_send_ris(), now want to read
	// 2. wrote zero bytes, want to read (came directly from libti2cit_m_isr_nofifo_send())
	if (status & I2C_MIMR_IM) {
		isr_cb = 0;
		libti2cit_m_isr_finish(I2C_MIMR_STOPIM);	// signal all done
	}
}

static void libti2cit_m_isr_nofifo_send_ris(uint32_t base, uint32_t status) {
	if (!status & I2C_MIMR_IM) return;
	uint32_t cmd, addr = HWREG(base + I2C_O_MSA);
	if (isr_len) {
		HWREG(base + I2C_O_MDR) = *(isr_buf++); // a.k.a. ROM_I2CMasterDataPut()
		cmd = (--isr_len | (addr & 1)) ? I2C_MASTER_CMD_BURST_SEND_CONT : I2C_MASTER_CMD_BURST_SEND_FINISH;
	} else if (addr & 1) {
		isr_cb = libti2cit_m_isr_nofifo_done_ris;
		cmd = I2C_MASTER_CMD_BURST_RECEIVE_START;
	} else {
		isr_cb = libti2cit_m_isr_nofifo_done_ris_stopris;
		return;
	}
	ROM_I2CMasterControl(base, cmd);
}

/* see description in libti2cit.h
 */
void libti2cit_m_isr_nofifo_send(uint32_t base, uint8_t addr, uint8_t len, const uint8_t * buf, libti2cit_status_cb cb) {
	uint8_t cmd = I2C_MASTER_CMD_QUICK_COMMAND;
	isr_cb = libti2cit_m_isr_nofifo_done_ris_stopris;	// case 1: len == 0 && (addr & 1) == 0

	if (len) {
		cmd = I2C_MASTER_CMD_BURST_SEND_START;
		isr_cb = libti2cit_m_isr_nofifo_send_ris;	// case 2: len != 0

		// the tiva i2c hardware wants the first data byte before the i2c start condition is sent
		if (buf) HWREG(base + I2C_O_MDR) = *(buf++); // a.k.a. ROM_I2CMasterDataPut()
		len--;

	} else if (addr & 1) {
		cmd = I2C_MASTER_CMD_BURST_RECEIVE_START;
		isr_cb = libti2cit_m_isr_nofifo_done_ris;	// case 3: len == 0 && (addr & 1) == 1
	}

	union {
		uint8_t * p;
		const uint8_t * c;
	} deconstify;
	deconstify.c = buf;
	isr_buf = deconstify.p;	// even though buf is only ever read, isr_buf must not be const for libti2cit_m_isr_nofifo_recv()
	isr_len = len;
	isr_user_cb = cb;
	isr_user_read_cb = 0;
	HWREG(base + I2C_O_MSA) = addr;	// a.k.a. ROM_I2CMasterSlaveAddrSet()
	ROM_I2CMasterControl(base, cmd);
}

static void libti2cit_m_isr_nofifo_recv_cb(uint32_t base, uint32_t status) {
	if (!isr_len) {
		if (status & I2C_MIMR_STOPIM) {
			libti2cit_m_isr_finish(I2C_MIMR_STOPIM);	// signal all done
			return;
		}
		// wait for STOPIM
		return;
	}

	// wait for RIS
	if (!(status & I2C_MIMR_IM)) return;

	*(isr_buf++) = HWREG(base + I2C_O_MDR) /* a.k.a. ROM_I2CMasterDataGet() */;
	isr_len--;
	isr_nread++;
	ROM_I2CMasterControl(base, isr_len ? I2C_MASTER_CMD_BURST_RECEIVE_CONT : I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
}

/* see description in libti2cit.h
 */
void libti2cit_m_isr_nofifo_recv(uint32_t base, uint8_t len, uint8_t * buf, libti2cit_read_cb cb) {
	if (!len) {	// len must be non-zero
		ROM_I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_ERROR_STOP);
		//UARTsend("libti2cit_m_isr_nofifo_recv(0): halting.\r\n");
		for (;;);	// deliberately freeze here to make it easy to debug
		return;
	}

	isr_nread = 0;
	isr_buf = buf;
	isr_len = len;
	isr_user_cb = 0;
	isr_user_read_cb = cb;
	isr_cb = libti2cit_m_isr_nofifo_recv_cb;

	// first byte already received by i2c hardware: read it, then check len
	libti2cit_m_isr_nofifo_recv_cb(base, I2C_MIMR_IM);
}
