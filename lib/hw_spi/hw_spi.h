#pragma once

uint8_t hw_spi_set_br_value(uint8_t sck_idx);
uint8_t hw_spi_get_br_value(void);
void hw_spi_enable(void);
void hw_spi_disable(void);
void hw_spi_recover(void);
bool hw_spi_transmit(const uint8_t *tx, uint8_t *rx, uint32_t len, uint32_t t);