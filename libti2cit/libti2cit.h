/* Copyright (c) 2014 David Hubbard github.com/davidhubbard
 *
 * libti2cit: an improvement over the tiva i2c driverlib. Use driverlib to initialize hardware, then call these
 * functions for data I/O.
 *
 * TODO: slave mode, interrupt-driven instead of polled, i2c FIFO, and uDMA
 */

/* libti2cit_m_sync_send(): i2c send a buffer and do not return until the send is complete.
 *   addr bit 0 == 0 for write, == 1 for read
 *   if write && len == 0 then libti2cit_m_send() does a "quick_command": i2c start, 8-bit address, i2c stop
 *   if write && len > 0 then libti2cit_m_send() does an i2c write:       i2c start, 8-bit address, data bytes, i2c stop
 *   if addr bit 0 == 1 for read: do not send the i2c stop:               i2c start, 8-bit address, send any data bytes as specified by len
 *      call libti2cit_m_recv() to read data bytes and send i2c stop
 *      it is not possible to do a "quick_command" recv(), i.e. do NOT call recv(len == 0)
 *      the correct way: send(addr bit 0 == 1, len >= 0) followed by a recv(len > 0) -- does a start, send, repeated start, recv, stop
 *
 * returns 0=ack, or > 0 for error
 */
extern uint8_t libti2cit_m_sync_send(uint32_t base, uint8_t addr, uint32_t len, const uint8_t * buf);

/* libti2cit_m_sync_recv(): i2c receive a buffer and do not return until the receive is complete
 *   must be preceded by libti2cit_m_send() with addr bit 0 == 1 for read
 *   len must be non-zero
 *
 * i2c does not support an ack or nack from the slave during a read
 * received data may be all ff's if the slave froze because the i2c bus is open-drain
 */
extern uint8_t libti2cit_m_sync_recv(uint32_t base, uint32_t len, uint8_t * buf);

/* libti2cit_m_sync_recvpart(): i2c receive a buffer but do not send i2c STOP -- for when the length varies based on the data
 *   must be preceded by libti2cit_m_send() with addr bit 0 == 1 for read
 *   call this repeatedly to continue receiving
 *   finally, call with len == 0 to send i2c STOP
 *   you MUST NOT call with len == 0 immediately after the libti2cit_m_send() which sets up the i2c bus (first receive some bytes!)
 *   you SHOULD always call with len == 0 to complete the bus transaction, and MUST NOT call any other functions until you do
 */
extern uint8_t libti2cit_m_sync_recvpart(uint32_t base, uint32_t len, uint8_t * buf);





/* libti2cit_int_st: isr context structure
 * this struct is used by the isr code to automatically handle the sending / receiving steps for you
 * tip: create your own struct and make libti2cit_int_st the first member
 *      then cast between libti2cit_int_st * and your struct when calling libti2cit and when getting callbacks
 *
 * you MUST fill in the fields required before calling the functions below
 * you MUST NOT read or write to fields in private_ (or your application will break badly because these fields will change)
 * except, you MUST initialize the entire libti2cit_int_st to 0 when it is first created
 */
typedef struct libti2cit_int_st_ libti2cit_int_st;
typedef void (* libti2cit_status_cb)(libti2cit_int_st * st, uint32_t status);
struct libti2cit_int_st_ {
	uint32_t base;
	uint8_t * buf;
	libti2cit_status_cb user_cb;
	uint32_t nread;
	uint32_t len;
	uint8_t addr;

	void * private_;
};

/* libti2cit_int_clear() reads I2C_O_MMIS then writes to I2C_O_MICR to acknowledge the interrupt
 * this is automatically called by libti2cit_m_isr_isr()
 * do NOT call this in your interrupt handler isr function - it is already taken care of
 * you probably want to call it during your setup code, before IntMasterEnable(), to clear out any stray interrupts
 *
 * you MUST fill in st->base
 */
extern uint32_t libti2cit_int_clear(libti2cit_int_st * st);

#define LIBTI2CIT_ISR_UNEXPECTED (0x10000000)

/* libti2cit_m_isr_isr(): you MUST call libti2cit_m_isr_isr() and pass it the same libti2cit_int_st when the I2C interrupt happens
 * each I2C base has its own interrupt handler in the interrupt vector table - install your own handler and call libti2cit_m_isr_isr()
 * from your handler
 *
 * returns I2C_O_MMIS bitwise ored with LIBTI2CIT_ISR_UNEXPECTED (an interrupt happened that was not expected)
 */
extern uint32_t libti2cit_m_isr_isr(libti2cit_int_st * st);

/* libti2cit_m_isr_nofifo_send(): i2c send a buffer and call user_cb when complete
 *   fill in libti2cit_int_st exactly like libti2cit_m_sync_send() arguments:
 *   you MUST fill in base, addr, len, buf, and user_cb in libti2cit_int_st
 *
 * on success: calls user_cb(status = I2C_MIMR_IM)
 * on failure: calls user_cb(status = I2C_MIMR_NACKIM)
 *   len and nread will be corrupted and you should not read its contents
 *   base, addr, buf, and user_cb will be unchanged
 *
 * DO NOT use libti2cit_int_st for multiple send()s and recv()s at once and DO NOT use the same base address for multiple libti2cit_int_st
 * DO reuse the same libti2cit_int_st when doing send()s and recv()s in sequence
 *   only read or write to the libti2cit_int_st in user_cb and after user_cb has been called
 */
extern void libti2cit_m_isr_nofifo_send(libti2cit_int_st * st);

/* libti2cit_m_isr_nofifo_recv(): i2c receive a buffer and call user_cb when complete
 *   fill in libti2cit_int_st exactly like libti2cit_m_sync_recv() arguments:
 *   you MUST fill in base, addr, len, buf, and user_cb in libti2cit_int_st
 *
 * on success: calls user_cb(status = I2C_MIMR_IM)
 * on failure: calls user_cb(status = I2C_MIMR_NACKIM)
 *   len will be corrupted and you should not read its contents
 *   base, addr, buf, and user_cb will be unchanged
 *   nread will contain the number of bytes received -- it is possible to receive a few bytes and also fail with I2C_MIMR_NACKIM
 *
 * DO NOT use libti2cit_int_st for multiple send()s and recv()s at once and DO NOT use the same base address for multiple libti2cit_int_st
 * DO reuse the same libti2cit_int_st when doing send()s and recv()s in sequence
 *   only read or write to the libti2cit_int_st in user_cb and after user_cb has been called
 */
extern void libti2cit_m_isr_nofifo_recv(libti2cit_int_st * st);

/* libti2cit_m_isr_nofifo_recvpart(): i2c receive a buffer but do not send i2c STOP -- for when the length varies based on the data
 *   fill in libti2cit_int_st exactly like libti2cit_m_sync_recvpart() arguments:
 *   you MUST fill in base, addr, len, buf, and user_cb in libti2cit_int_st
 *
 * on success: calls user_cb(status = I2C_MIMR_IM)
 * on failure: calls user_cb(status = I2C_MIMR_NACKIM)
 *   len will be corrupted and you should not read its contents
 *   base, addr, buf, and user_cb will be unchanged
 *   nread will contain the number of bytes received -- it is possible to receive a few bytes and also fail with I2C_MIMR_NACKIM
 *
 * DO NOT use libti2cit_int_st for multiple send()s and recv()s at once and DO NOT use the same base address for multiple libti2cit_int_st
 * DO reuse the same libti2cit_int_st when doing send()s and recv()s in sequence
 *   only read or write to the libti2cit_int_st in user_cb and after user_cb has been called
 */
extern void libti2cit_m_isr_nofifo_recvpart(libti2cit_int_st * st);



/* libti2cit_m_isr_send(): i2c send a buffer and call user_cb when complete
 *   fill in libti2cit_int_st exactly like libti2cit_m_sync_send() arguments:
 *   you MUST fill in base, addr, len, buf, and user_cb in libti2cit_int_st
 *
 * on success: calls user_cb(status = I2C_MIMR_IM)
 * on failure: calls user_cb(status = I2C_MIMR_NACKIM)
 *   len and nread will be corrupted and you should not read its contents
 *   base, addr, buf, and user_cb will be unchanged
 *
 * DO NOT use libti2cit_int_st for multiple send()s and recv()s at once and DO NOT use the same base address for multiple libti2cit_int_st
 * DO reuse the same libti2cit_int_st when doing send()s and recv()s in sequence
 *   only read or write to the libti2cit_int_st in user_cb and after user_cb has been called
 */
extern void libti2cit_m_isr_send(libti2cit_int_st * st);

/* libti2cit_m_isr_recv(): i2c receive a buffer and call user_cb when complete
 *   fill in libti2cit_int_st exactly like libti2cit_m_sync_recv() arguments:
 *   you MUST fill in base, addr, len, buf, and user_cb in libti2cit_int_st
 *
 * on success: calls user_cb(status = I2C_MIMR_IM)
 * on failure: calls user_cb(status = I2C_MIMR_NACKIM)
 *   len will be corrupted and you should not read its contents
 *   base, addr, buf, and user_cb will be unchanged
 *   nread will contain the number of bytes received -- it is possible to receive a few bytes and also fail with I2C_MIMR_NACKIM
 *
 * DO NOT use libti2cit_int_st for multiple send()s and recv()s at once and DO NOT use the same base address for multiple libti2cit_int_st
 * DO reuse the same libti2cit_int_st when doing send()s and recv()s in sequence
 *   only read or write to the libti2cit_int_st in user_cb and after user_cb has been called
 */
extern void libti2cit_m_isr_recv(libti2cit_int_st * st);

/* libti2cit_m_isr_recvpart(): i2c receive a buffer but do not send i2c STOP -- for when the length varies based on the data
 *   fill in libti2cit_int_st exactly like libti2cit_m_sync_recvpart() arguments:
 *   you MUST fill in base, addr, len, buf, and user_cb in libti2cit_int_st
 *
 * on success: calls user_cb(status = I2C_MIMR_IM)
 * on failure: calls user_cb(status = I2C_MIMR_NACKIM)
 *   len will be corrupted and you should not read its contents
 *   base, addr, buf, and user_cb will be unchanged
 *   nread will contain the number of bytes received -- it is possible to receive a few bytes and also fail with I2C_MIMR_NACKIM
 *
 * DO NOT use libti2cit_int_st for multiple send()s and recv()s at once and DO NOT use the same base address for multiple libti2cit_int_st
 * DO reuse the same libti2cit_int_st when doing send()s and recv()s in sequence
 *   only read or write to the libti2cit_int_st in user_cb and after user_cb has been called
 */
extern void libti2cit_m_isr_recvpart(libti2cit_int_st * st);
