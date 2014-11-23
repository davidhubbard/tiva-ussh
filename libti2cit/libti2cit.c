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
uint8_t libti2cit_m_sync_send(uint32_t base, uint8_t addr, uint32_t len, const uint8_t * buf) {
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
			HWREG(base + I2C_O_MIMR) |= I2C_MIMR_STARTIM;	// a START actually will NOT happen, no interrupt will fire: abuse this bit to signal a repeated start for libti2cit_m_sync_recvpart()
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
uint8_t libti2cit_m_sync_recv(uint32_t base, uint32_t len, uint8_t * buf) {
	HWREG(base + I2C_O_MIMR) &= ~I2C_MIMR_STARTIM;	// bit was set in case libti2cit_m_sync_recvpart() would be called, clear it now

	// first byte was already received by i2c state machine
	if (!len) {	// len cannot be zero. first byte was already received so len is at least 1!
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
uint8_t libti2cit_m_sync_recvpart(uint32_t base, uint32_t len, uint8_t * buf)
{
	uint32_t mimr = HWREG(base + I2C_O_MIMR);
	if (mimr & I2C_MIMR_STARTIM) {	// if this is the first time calling libti2cit_m_sync_recvpart()
		HWREG(base + I2C_O_MIMR) = mimr & ~I2C_MIMR_STARTIM;
		if (!len) {	// first receive some bytes!
			ROM_I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_ERROR_STOP);
			return 1;
		}

		// first byte already received by i2c hardware
		*(buf++) = HWREG(base + I2C_O_MDR); /* a.k.a. ROM_I2CMasterDataGet() */
		len--;
	} else if (!len) {
		libti2cit_m_continue(base, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
		libti2cit_mris_wait(base, I2C_MRIS_STOPRIS | I2C_MRIS_RIS, 0);
		return 0;
	}

	while (len) {
		libti2cit_m_continue(base, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
		*(buf++) = HWREG(base + I2C_O_MDR); /* a.k.a. ROM_I2CMasterDataGet() */
		len--;
	}
	return 0;
}





















static void libti2cit_m_isr_set_isr_cb(libti2cit_int_st * st, libti2cit_status_cb cb)
{
	union {
		void * as_void;
		libti2cit_status_cb as_cb;
	} punned;
	punned.as_cb = cb;
	st->private_ = punned.as_void;
}
static void libti2cit_m_isr_call_isr_cb(libti2cit_int_st * st, uint32_t status)
{
	union {
		void * as_void;
		libti2cit_status_cb as_cb;
	} punned;
	punned.as_void = st->private_;
	punned.as_cb(st, status);
}

/* see description in libti2cit.h
 */
uint32_t libti2cit_int_clear(libti2cit_int_st * st)
{
	uint32_t status = HWREG(st->base + I2C_O_MMIS);

	HWREG(st->base + I2C_O_MICR) = status;
	if (status == I2C_MMIS_MIS) {
		/* Workaround for I2C master interrupt clear errata for rev B Tiva
		 * devices.  For later devices, this write is ignored and therefore
		 * harmless other than the slight performance hit.
		 */
		HWREG(st->base + I2C_O_MMIS) = I2C_MMIS_MIS;
	}

	return status;
}

static uint32_t libti2cit_m_isr_finish(libti2cit_int_st * st, uint32_t status)
{
	// this ends internal isr_cb action, unless this is a NACK
	// if this is a NACK, the isr_cb is still needed for the next interrupt the hardware generates
	if (!(status & I2C_MIMR_NACKIM)) libti2cit_m_isr_set_isr_cb(st, 0);

	if (!st->user_cb) {
		//UARTsend("!isr_user_cb\r\n");
		return status | LIBTI2CIT_ISR_UNEXPECTED;
	}

	if (st->user_cb) st->user_cb(st, status);
	return status;
}

/* see description in libti2cit.h
 */
uint32_t libti2cit_m_isr_isr(libti2cit_int_st * st)
{
	uint32_t status = libti2cit_int_clear(st);

	if (status & I2C_MIMR_NACKIM) {
		if (!st->user_cb) {
			//UARTsend("!isr_user_cb\r\n");
			status |= LIBTI2CIT_ISR_UNEXPECTED;	// signal UNEXPECTED
		}
		return libti2cit_m_isr_finish(st, status);
	}

	if (!st->private_) {
		// isr_cb was not set...UNEXPECTED
		//UARTsend("!isr_cb\r\n");
		status |= LIBTI2CIT_ISR_UNEXPECTED;
		return libti2cit_m_isr_finish(st, status);
	}

	// normal isr_cb
	libti2cit_m_isr_call_isr_cb(st, status);
	return status;
}

static void libti2cit_m_isr_nofifo_done_ris_stopris(libti2cit_int_st * st, uint32_t status)
{
	// this happens in 2 cases:
	// 1. finished writing 1+ bytes in libti2cit_m_isr_nofifo_send_ris(), now want to stop
	// 2. finished writing 0 bytes, want to stop (came directly from libti2cit_m_isr_nofifo_send())
	if (status & I2C_MIMR_STOPIM) libti2cit_m_isr_finish(st, I2C_MIMR_STOPIM);	// signal all done
}
static void libti2cit_m_isr_nofifo_done_ris(libti2cit_int_st * st, uint32_t status)
{
	// this happens in 2 cases:
	// 1. finished writing 1+ bytes in libti2cit_m_isr_nofifo_send_ris(), now want to read
	// 2. wrote zero bytes, want to read (came directly from libti2cit_m_isr_nofifo_send())
	if (status & I2C_MIMR_IM) libti2cit_m_isr_finish(st, I2C_MIMR_STOPIM);	// signal all done
}

static void libti2cit_m_isr_nofifo_send_ris(libti2cit_int_st * st, uint32_t status)
{
	if (!status & I2C_MIMR_IM) return;
	uint32_t cmd;
	if (st->nread < st->len) {
		HWREG(st->base + I2C_O_MDR) = st->buf[st->nread]; // a.k.a. ROM_I2CMasterDataPut()
		st->nread++;
		cmd = ((st->nread < st->len) || (st->addr & 1)) ? I2C_MASTER_CMD_BURST_SEND_CONT : I2C_MASTER_CMD_BURST_SEND_FINISH;
	} else if (st->addr & 1) {
		libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_done_ris);
		HWREG(st->base + I2C_O_MIMR) |= I2C_MIMR_STARTIM;	// a START actually will NOT happen, no interrupt will fire: abuse this bit to signal a repeated start for libti2cit_m_sync_recvpart()
		cmd = I2C_MASTER_CMD_BURST_RECEIVE_START;
	} else {
		libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_done_ris_stopris);
		return;
	}
	ROM_I2CMasterControl(st->base, cmd);
}

/* see description in libti2cit.h
 */
void libti2cit_m_isr_nofifo_send(libti2cit_int_st * st)
{
	uint8_t cmd = I2C_MASTER_CMD_QUICK_COMMAND;
	libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_done_ris_stopris);	// case 1: len == 0 && (addr & 1) == 0

	st->nread = 0;
	if (st->len) {
		cmd = I2C_MASTER_CMD_BURST_SEND_START;
		libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_send_ris);	// case 2: len != 0

		// the tiva i2c hardware wants the first data byte before the i2c start condition is sent
		if (st->buf) HWREG(st->base + I2C_O_MDR) = st->buf[0]; // a.k.a. ROM_I2CMasterDataPut()
		st->nread++;

	} else if (st->addr & 1) {
		cmd = I2C_MASTER_CMD_BURST_RECEIVE_START;
		HWREG(st->base + I2C_O_MIMR) |= I2C_MIMR_STARTIM;	// a START actually will NOT happen, no interrupt will fire: abuse this bit to signal a repeated start for libti2cit_m_sync_recvpart()
		libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_done_ris);	// case 3: len == 0 && (addr & 1) == 1
	}

	HWREG(st->base + I2C_O_MSA) = st->addr;	// a.k.a. ROM_I2CMasterSlaveAddrSet()
	ROM_I2CMasterControl(st->base, cmd);
}

static void libti2cit_m_isr_nofifo_recv_cb(libti2cit_int_st * st, uint32_t status)
{
	if (st->nread >= st->len) {
		// wait for STOPIM
		if (!(status & I2C_MIMR_STOPIM)) return;

		libti2cit_m_isr_finish(st, I2C_MIMR_STOPIM);	// signal all done
		return;
	}

	// wait for RIS
	if (!(status & I2C_MIMR_IM)) return;

	st->buf[st->nread] = HWREG(st->base + I2C_O_MDR) /* a.k.a. ROM_I2CMasterDataGet() */;
	st->nread++;
	ROM_I2CMasterControl(st->base, (st->nread < st->len) ? I2C_MASTER_CMD_BURST_RECEIVE_CONT : I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
}

/* see description in libti2cit.h
 */
void libti2cit_m_isr_nofifo_recv(libti2cit_int_st * st)
{
	HWREG(st->base + I2C_O_MIMR) &= ~I2C_MIMR_STARTIM;	// bit was set in case libti2cit_m_sync_recvpart() would be called, clear it now

	if (!st->len) {	// len cannot be zero. first byte was already received so len is at least 1!
		ROM_I2CMasterControl(st->base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_ERROR_STOP);
		//UARTsend("libti2cit_m_isr_nofifo_recv(0): halting.\r\n");
		for (;;);	// deliberately freeze here to make it easy to debug
		return;
	}
	st->nread = 0;

	libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_recv_cb);

	// first byte already received by i2c hardware: read it, then check len
	libti2cit_m_isr_nofifo_recv_cb(st, I2C_MIMR_IM);
}

static void libti2cit_m_isr_nofifo_recvpart_cb(libti2cit_int_st * st, uint32_t status)
{
	// wait for RIS
	if (!(status & I2C_MIMR_IM)) return;

	if (st->nread >= st->len) {
		libti2cit_m_isr_finish(st, I2C_MIMR_STOPIM);	// signal all done (but no i2c STOP occurred)
		return;
	}

	st->buf[st->nread] = HWREG(st->base + I2C_O_MDR) /* a.k.a. ROM_I2CMasterDataGet() */;
	st->nread++;
	ROM_I2CMasterControl(st->base, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
}

/* see description in libti2cit.h
 */
void libti2cit_m_isr_nofifo_recvpart(libti2cit_int_st * st)
{
	st->nread = 0;
	libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_recvpart_cb);

	uint32_t mimr = HWREG(st->base + I2C_O_MIMR);
	if (mimr & I2C_MIMR_STARTIM) {	// if this is the first time calling libti2cit_m_sync_recvpart()
		HWREG(st->base + I2C_O_MIMR) = mimr & ~I2C_MIMR_STARTIM;
		if (!st->len) {	// first receive some bytes!
			ROM_I2CMasterControl(st->base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_ERROR_STOP);
			//UARTsend("libti2cit_m_isr_nofifo_recv(0): halting.\r\n");
			for (;;);	// deliberately freeze here to make it easy to debug
			return;
		}

		// first byte already received by i2c hardware
		libti2cit_m_isr_nofifo_recv_cb(st, I2C_MIMR_IM);
	} else if (!st->len) {
		libti2cit_m_isr_set_isr_cb(st, libti2cit_m_isr_nofifo_recv_cb);
		ROM_I2CMasterControl(st->base, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
		return;
	}

	ROM_I2CMasterControl(st->base, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
	return;
}
