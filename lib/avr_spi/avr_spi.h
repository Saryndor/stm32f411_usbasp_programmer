/* 
   AVR - Serial Programming Instructions

*/
#pragma once

#include "stdio.h"
#include "stdint.h"
#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
// #include "delay.h"


#define AVR_RESET_PIN      GPIO13
#define AVR_RESET_PORT     GPIOB
#define AVR_RESET_RCC_GPIO RCC_GPIOB

#define AVR_OE_PIN GPIO4
#define AVR_OE_PORT GPIOA
#define AVR_OE_RCC_GPIO RCC_GPIOA

#define AVR_MISO_PWR_PIN GPIO5
#define AVR_MISO_PWR_PORT GPIOA
#define AVR_MISO_PWR_RCC_GPIO RCC_GPIOA


static inline void avr_spi_reset_pin_init() {
   rcc_periph_clock_enable(AVR_RESET_RCC_GPIO);
   gpio_mode_setup(AVR_RESET_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AVR_RESET_PIN);
   gpio_set_output_options(AVR_RESET_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AVR_RESET_PIN);
   gpio_set(AVR_RESET_PORT, AVR_RESET_PIN); 
}

static inline void avr_spi_reset_pin_set() {
   gpio_set(AVR_RESET_PORT, AVR_RESET_PIN);
}

static inline void avr_spi_reset_pin_clear() {
   gpio_clear(AVR_RESET_PORT, AVR_RESET_PIN);
}

static inline void avr_spi_oe_pin_init() {
   rcc_periph_clock_enable(AVR_OE_RCC_GPIO);
   gpio_mode_setup(AVR_OE_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AVR_OE_PIN);
   gpio_set_output_options(AVR_OE_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AVR_OE_PIN);
   gpio_clear(AVR_OE_PORT, AVR_OE_PIN);
}

static inline void avr_spi_oe_pin_set() {
   gpio_set(AVR_OE_PORT, AVR_OE_PIN);
}

static inline void avr_spi_oe_pin_reset() {
   gpio_clear(AVR_OE_PORT, AVR_OE_PIN);
}

static inline void avr_spi_miso_pwr_pin_init() {
   rcc_periph_clock_enable(AVR_MISO_PWR_RCC_GPIO);
   gpio_mode_setup(AVR_MISO_PWR_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AVR_MISO_PWR_PIN);
   gpio_set_output_options(AVR_MISO_PWR_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AVR_MISO_PWR_PIN);
   gpio_clear(AVR_MISO_PWR_PORT, AVR_MISO_PWR_PIN);

}

static inline void avr_spi_miso_pwr_pin_set() {
   gpio_set(AVR_MISO_PWR_PORT, AVR_MISO_PWR_PIN);
}

static inline void avr_spi_miso_pwr_pin_reset() {
   gpio_clear(AVR_MISO_PWR_PORT, AVR_MISO_PWR_PIN);
}


bool avr_spi_set_spi(uint8_t sck_idx);
void avr_spi_enable(void);
void avr_spi_disable(void);
uint8_t* avr_spi_get_tx_ptr(void);
uint8_t* avr_spi_get_rx_ptr(void);
uint8_t* avr_spi_transmit(uint8_t* tx);
void avr_spi_reset_ext_addr(void);
void avr_spi_reset_sig(void);
uint32_t avr_spi_get_sig(void);
void avr_spi_check_ext_addr(uint32_t byte_addr);
uint8_t avr_spi_enableprog(void);
uint8_t avr_spi_read_flash_byte(uint32_t byte_addr);
void avr_spi_write_flash_byte(uint32_t byte_addr, uint8_t data);
void avr_spi_flash_page_commit(uint32_t byte_addr);
uint8_t avr_spi_read_eeprom_byte(uint32_t byte_addr);
void avr_spi_write_eeprom_byte(uint32_t byte_addr, uint8_t data);
void avr_spi_eeprom_page_commit(uint32_t byte_addr);
void avr_spi_chip_erase(void);
