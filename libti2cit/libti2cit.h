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
 *      (call libti2cit_m_recv() to read data bytes and send i2c stop.
 *      it is not possible to do a "quick_command" read, i.e.
 *      with addr bit 0 == 1, but a len == 0 write followed by a
 *      read still works correctly.)
 *
 * returns 0=ack, or > 0 for error
 */
extern uint8_t libti2cit_m_sync_send(uint32_t base, uint8_t addr, uint8_t len, const uint8_t * buf);

/* libti2cit_m_sync_recv(): i2c receive a buffer and do not return until the receive is complete.
 *   must be preceded by libti2cit_m_send() with addr bit 0 == 1 for read
 *   len must be non-zero
 *
 * i2c does not support an ack or nack from the slave during a read
 * (received data may be all ff's if the slave froze because the i2c bus is open-drain)
 */
extern uint8_t libti2cit_m_sync_recv(uint32_t base, uint8_t len, uint8_t * buf);


extern uint32_t isr_debug_level;
uint32_t libti2cit_int_clear(uint32_t base);

typedef void (* libti2cit_status_cb)(uint32_t status);
typedef void (* libti2cit_read_cb)(uint32_t status, uint32_t nread);
extern void libti2cit_m_isr_nofifo_send(uint32_t base, uint8_t addr, uint8_t len, const uint8_t * buf, libti2cit_status_cb cb);
extern void libti2cit_m_isr_nofifo_recv(uint32_t base, uint8_t len, uint8_t * buf, libti2cit_read_cb cb);
extern void libti2cit_isr_clear();

#define LIBTI2CIT_ISR_UNEXPECTED (0x10000000)
#define LIBTI2CIT_ISR_MCS_ARBLST (0x20000000)
extern uint32_t libti2cit_m_isr_isr(uint32_t base);
