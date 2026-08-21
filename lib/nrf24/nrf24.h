#ifndef NRF24_H
#define NRF24_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

void nrf24_setup(void);
bool nrf24_init_tx (uint8_t payload_size);
bool nrf24_init_rx (uint8_t payload_size);
uint8_t spi_write_reg(uint8_t reg, uint8_t data);
uint8_t spi_multi_write_reg(uint8_t reg, uint8_t *data, int size);
uint8_t spi_read_reg(uint8_t reg);
uint8_t spi_command(uint8_t command);
bool nrf24_send(const void *buffer, uint8_t length);
bool nrf24_receive(void *buff, uint8_t length);
void check_ce_high(void);

#ifdef __cplusplus
}
#endif

#endif