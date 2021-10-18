#ifndef EEPROM_H_
#define EEPROM_H_

// Pointer to user called eeprom error function
typedef void (*eeprom_err_callback)(void);

// Set eeprom error callback function
void eeprom_set_err_callback(eeprom_err_callback __function);

// Init eeprom
void eeprom_init(void);

// Send byte to spi, return rx byte
uint8_t send_spi(const uint8_t __byte);

// Return 1 if EEPROM is ready for a new read/write operation, 0 if not
uint8_t eeprom_is_ready(void);

// Loops until the eeprom is no longer busy
void eeprom_busy_wait(void);

// Read one byte from EEPROM address __p
uint8_t eeprom_read_byte(const uint8_t *__p);

// Read one 16-bit word (little endian) from EEPROM address __p
uint16_t eeprom_read_word(const uint16_t *__p);

// Write a byte __value to EEPROM address __p
void eeprom_write_byte(uint8_t *__p, uint8_t __value);

// Write a word __value to EEPROM address __p
void eeprom_write_word(uint16_t *__p, uint16_t __value);

// Update a byte __value to EEPROM address __p
void eeprom_update_byte(uint8_t *__p, uint8_t __value);

// Update a word __value to EEPROM address __p
void eeprom_update_word(uint16_t *__p, uint16_t __value);

#endif /* EEPROM_H_ */
