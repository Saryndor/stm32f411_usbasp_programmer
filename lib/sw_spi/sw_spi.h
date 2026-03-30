#pragma once

uint32_t sw_spi_set_delay(uint8_t sck_idx);
uint32_t sw_spi_get_delay(void);
void sw_spi_enable(void);
void sw_spi_disable(void);
void sw_spi_transmit(const uint8_t *tx, uint8_t *rx, uint32_t len);