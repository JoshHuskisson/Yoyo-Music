#ifndef I2C_H
#define I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* 2. Constants and Macros */


/* 3. Data Structures */


/* 4. Function Prototypes (Declarations only) */
void i2c_setup(void);
int i2c_write_read(uint8_t addr, uint8_t reg, uint8_t *buff, uint32_t size);
bool i2c_reg_write(uint8_t addr, uint8_t reg, uint8_t data);
int error_check(void);

#ifdef __cplusplus
}
#endif

#endif