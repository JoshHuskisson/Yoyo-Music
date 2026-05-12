#ifndef I2C_H
#define I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/* 2. Constants and Macros */


/* 3. Data Structures */


/* 4. Function Prototypes (Declarations only) */
void setup(void);
int write_read(uint8_t addr, uint8_t reg, uint8_t *buff, int size);

#ifdef __cplusplus
}
#endif

#endif