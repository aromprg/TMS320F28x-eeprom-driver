#include <stdint.h>
#include "DSP280x_Device.h"
#include "DSP280x_Examples.h"
#include "eeprom.h"

// spi config & control registers
#define SPI_REGS_EEPROM	SpibRegs	

// chip select
#define EE_CS_DELAY	DELAY_US(0.25L)
#define CS_EEPROM_LOW	GpioDataRegs.GPACLEAR.bit.GPIO28 = 1
#define CS_EEPROM_HIGH	EE_CS_DELAY; GpioDataRegs.GPASET.bit.GPIO28 = 1; EE_CS_DELAY

#define MAX_BUSY_WAIT	400 // spi_clk 2MHz -> 400; 4MHz -> 800

// eeprom cmd
#define EE_WRSR		0x01
#define EE_WRITE	0x02
#define EE_READ		0x03
#define EE_WRDI		0x04
#define EE_RDSR		0x05
#define EE_WREN		0x06

eeprom_err_callback eeprom_err_callback_f = 0;

// Set eeprom error callback function
void eeprom_set_err_callback(eeprom_err_callback __function) {
	eeprom_err_callback_f = __function;
}

// Init eeprom
void eeprom_init(void) {
	
	// GPIO cfg
	
	EALLOW;
	
//			SPICLKB
//
//	GPIO-26 - PIN FUNCTION = --Spare--
//	GpioCtrlRegs.GPAQSEL2.bit.GPIO26 = 3;	// Asynch input
	GpioCtrlRegs.GPAPUD.bit.GPIO26 = 1;	// uncomment if --> Disable pull-up
//	GpioCtrlRegs.GPAPUD.bit.GPIO26 = 0;	// uncomment if --> Enable pull-up
	GpioDataRegs.GPACLEAR.bit.GPIO26 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO26 = 1;	// uncomment if --> Set High initially
	GpioCtrlRegs.GPADIR.bit.GPIO26 = 1;	// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 3;	// 0=GPIO,  1=ECAP3,  2=EQEP2I,  3=SPICLKB

//			SPISIMOB
//
//	GPIO-12 - PIN FUNCTION = --Spare--
//	GpioCtrlRegs.GPAQSEL1.bit.GPIO12 = 3;	// Asynch input
	GpioCtrlRegs.GPAPUD.bit.GPIO12 = 1;	// uncomment if --> Disable pull-up
//	GpioCtrlRegs.GPAPUD.bit.GPIO12 = 0;	// uncomment if --> Enable pull-up
	GpioDataRegs.GPACLEAR.bit.GPIO12 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO12 = 1;	// uncomment if --> Set High initially
	GpioCtrlRegs.GPADIR.bit.GPIO12 = 1;	// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 3;	// 0=GPIO,  1=TZ1,  2=CANTXB,  3=SPISIMOB

//			SPISOMIB
//
//	GPIO-13 - PIN FUNCTION = --Spare--
//	GpioCtrlRegs.GPAQSEL1.bit.GPIO13 = 3;	// Asynch input
//	GpioCtrlRegs.GPAPUD.bit.GPIO13 = 1;	// uncomment if --> Disable pull-up
	GpioCtrlRegs.GPAPUD.bit.GPIO13 = 0;	// uncomment if --> Enable pull-up
//	GpioDataRegs.GPACLEAR.bit.GPIO13 = 1;	// uncomment if --> Set Low initially
//	GpioDataRegs.GPASET.bit.GPIO13 = 1;	// uncomment if --> Set High initially
	GpioCtrlRegs.GPADIR.bit.GPIO13 = 0;	// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 3;	// 0=GPIO,  1=TZ2,  2=CANRXB,  3=SPISOMIB

//			CS pin
//
//	GPIO-28 - PIN FUNCTION = --Spare--
//	GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;	// Asynch input
	GpioCtrlRegs.GPAPUD.bit.GPIO28 = 1;	// uncomment if --> Disable pull-up
//	GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;	// uncomment if --> Enable pull-up
//	GpioDataRegs.GPACLEAR.bit.GPIO28 = 1;	// uncomment if --> Set Low initially
	GpioDataRegs.GPASET.bit.GPIO28 = 1;	// uncomment if --> Set High initially
	GpioCtrlRegs.GPADIR.bit.GPIO28 = 1;	// 1=OUTput,  0=INput
	GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 0;	// 0=GPIO,  1=SCIRXDA,  2=Reserved,  3=TZ5
	
	EDIS;
	
	
	// SPI cfg
	
	SPI_REGS_EEPROM.SPICCR.all = 0x0007;		// SWRESET, rising edge, loop back mode disabled, 8 data bit
	SPI_REGS_EEPROM.SPIFFTX.bit.SPIFFENA = 0;	// TX FIFO disable
	SPI_REGS_EEPROM.SPIFFRX.bit.RXFFIENA = 0;	// RX FIFO disable
	SPI_REGS_EEPROM.SPICTL.all = 0x000E;		// master mode, SPICLK signal delayed by one half-cycle, enable talk, SPI interrupt disabled
	SPI_REGS_EEPROM.SPIBRR = 11;			// LSPCLK=100MHz/4=25MHz; SPI Baud Rate = 25MHz/(11+1)=2,08333MHz
	SPI_REGS_EEPROM.SPIPRI.bit.FREE = 1;		// Set so breakpoints don't disturb xmission
	SPI_REGS_EEPROM.SPICCR.bit.SPISWRESET = 1;	// init done

	eeprom_busy_wait();
}

// Send byte to spi, return rx byte
uint8_t send_spi(const uint8_t __byte) {

	SPI_REGS_EEPROM.SPITXBUF = ((uint16_t) __byte & 0xFF) << 8;

	while (!SPI_REGS_EEPROM.SPISTS.bit.INT_FLAG) {
		;
	}

	return (SPI_REGS_EEPROM.SPIRXBUF & 0xFF);
}

// Return 1 if EEPROM is ready for a new read/write operation, 0 if not
uint8_t eeprom_is_ready(void) {
	uint8_t tmp;

	CS_EEPROM_LOW;
	send_spi(EE_RDSR);
	tmp = send_spi(0x00);
	CS_EEPROM_HIGH;

	return !(tmp & 0x01);
}

// Loops until the eeprom is no longer busy
void eeprom_busy_wait(void) {
	uint16_t i;

	for (i = 0; i < MAX_BUSY_WAIT; i++) {
		if (eeprom_is_ready()) {
			return;
		}
	}

	// busy long time
	if (eeprom_err_callback_f) {
		eeprom_err_callback_f();
	}
}

// Read one byte from EEPROM address __p
uint8_t eeprom_read_byte(const uint8_t *__p) {

	uint32_t addr = (uint32_t)__p;
	uint8_t byte;

	CS_EEPROM_LOW;
	send_spi(EE_READ);
	send_spi(addr >> 8);
	send_spi(addr);
	byte = send_spi(0x00);
	CS_EEPROM_HIGH;

	return byte;
}

// Read one 16-bit word (little endian) from EEPROM address __p
uint16_t eeprom_read_word(const uint16_t *__p) {

	uint16_t tmp = eeprom_read_byte((uint8_t *)__p);

	tmp |= (uint16_t)eeprom_read_byte((uint8_t *)(__p + 1)) << 8;
	return tmp;
}

// Write a byte __value to EEPROM address __p
void eeprom_write_byte(uint8_t *__p, uint8_t __value) {

	uint32_t addr = (uint32_t)__p;

	CS_EEPROM_LOW;
	send_spi(EE_WREN); // write enable
	CS_EEPROM_HIGH;

	CS_EEPROM_LOW;
	send_spi(EE_WRITE);

	send_spi(addr >> 8);
	send_spi(addr);
	send_spi(__value);
	CS_EEPROM_HIGH;

	eeprom_busy_wait();
}

// Write a word __value to EEPROM address __p
void eeprom_write_word(uint16_t *__p, uint16_t __value) {
	eeprom_write_byte((uint8_t *)__p, __value);
	eeprom_write_byte((uint8_t *)(__p + 1), __value >> 8);
}

// Update a byte __value to EEPROM address __p
void eeprom_update_byte(uint8_t *__p, uint8_t __value) {
	if (eeprom_read_byte(__p) != __value) {
		eeprom_write_byte(__p, __value);
	}
}

// Update a word __value to EEPROM address __p
void eeprom_update_word(uint16_t *__p, uint16_t __value) {

	uint16_t word = eeprom_read_word(__p);

	if ((word & 0xFF) != (__value & 0xFF)) {
		eeprom_write_byte((uint8_t *)__p, __value);
	}

	if ((word >> 8) != (__value >> 8)) {
		eeprom_write_byte((uint8_t *)(__p + 1), (__value >> 8));
	}
}
