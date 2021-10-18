# AT25xxx SPI-EEPROM driver for TI TMS320F28xxx CPU.

Designed as avr-gcc styled eeprom API functions.

# How to use:
In main init function write this:

```c
void init() {
    ...
    eeprom_set_err_callback(&eeprom_err);
    eeprom_init();
    ...
  }
```
eeprom_err() implemented as
```c
// call if eeprom invalid
void eeprom_err(void) {
    puts("eeprom_err");
    // TODO
}
```
###### Remark: TI C28 core has only integer types with a minimum width of 16 bits. For multi-platform compatibility this driver use `uint8_t` type. Add to your project: `typedef unsigned int uint8_t;`
