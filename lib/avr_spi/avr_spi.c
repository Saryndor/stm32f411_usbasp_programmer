#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "avr_spi.h"
#include "hw_spi.h"
#include "sw_spi.h"
#include "delay.h"
#include "avr_devices_isr.h"

static uint8_t  avr_spi_tx[4];
static uint8_t  avr_spi_rx[4];
static uint8_t  avr_spi_ext_addr;
static uint32_t avr_spi_sig;

static bool use_sw_spi = false;

typedef enum {
    FLASH_DELAY,
    EEPROM_DELAY
} avr_busy_mode_t;


/*
    sck_idx  0:   default  (hw_spi; 187.5 kHz)
    sck_idx  1:   0.50 kHz (sw_spi)
    sck_idx  2:   1.00 kHz (sw_spi)
    sck_idx  3:   2.00 kHz (sw_spi)
    sck_idx  4:   4.00 kHz (sw_spi)
    sck_idx  5:   8.00 kHz (sw_spi)
    sck_idx  6:  16.00 kHz (sw_spi)
    sck_idx  7:  32.00 kHz (sw_spi)
    sck_idx  8:  93.75 kHz (sw_spi)
    sck_idx  9: 187.50 kHz (hw_spi)
    sck_idx 10: 375.00 kHz (hw_spi)
    sck_idx 11: 750.00 kHz (hw_spi)
    sck_idx 12:   1.50 MHz (hw_spi)
    sck_idx 13:   3.00 MHz (hw_spi)
*/

bool avr_spi_set_spi(uint8_t sck_idx) {
    // Default
    if ( sck_idx == 0) {            // Default; Hardware-SPI
        use_sw_spi = false;
        hw_spi_set_br_value(9);
    } else if ( sck_idx < 9 ) {     // Software-SPI
        use_sw_spi = true;
        sw_spi_set_delay(sck_idx);
    } else if ( sck_idx < 14 ) {    // Hardware-SPI
        use_sw_spi = false;
        hw_spi_set_br_value(sck_idx);    
    } else {                        // Default; Hardware-SPI
        use_sw_spi = false;
        hw_spi_set_br_value(9);
    }
}

uint8_t* avr_spi_get_tx_ptr(void) {
    return avr_spi_tx;
}

void avr_spi_enable(void){
    avr_spi_miso_pwr_pin_set();
    if (use_sw_spi) {
        hw_spi_disable();
        sw_spi_enable();
    } else {
        sw_spi_disable();
        hw_spi_enable();
    }
}

void avr_spi_disable(void){
    if (use_sw_spi) 
        sw_spi_disable();
    else
        hw_spi_disable();

    avr_spi_miso_pwr_pin_reset();
}

uint8_t* avr_spi_get_rx_ptr(void) {
    return avr_spi_rx;
}

bool spi_tx_rx(const uint8_t* tx, uint8_t* rx, uint16_t len, uint16_t t)
{
  if (len == 0) return true;

  if (use_sw_spi) {
     sw_spi_transmit(tx, rx, len);
     return true;
  } 
  else {
     hw_spi_transmit(tx, rx, len, t);
     return true;
  }
}

bool avr_spi_wait_ready(avr_busy_mode_t mode)
{
    if (mode != FLASH_DELAY && mode != EEPROM_DELAY) {
        delay_ms(50);
        return true;
    }

    avr_spi_tx[0] = 0xF0;
    avr_spi_tx[1] = 0x00;
    avr_spi_tx[2] = 0x00;
    avr_spi_tx[3] = 0x00;
    
    uint8_t _delay, _mode;

    if ( mode == FLASH_DELAY ) {
        _delay = avr_device_isr_get_flash_delay_ms();
        _mode = avr_device_isr_get_flash_mode();
    } else {
        _delay = avr_device_isr_get_eeprom_delay_ms();
        _mode = avr_device_isr_get_eeprom_mode();
    }

    if (_mode != 0x41)
    {
        delay_ms(_delay);
        return true;
    }

    // base wait time
    delay_ms(_delay / 2);

    // Calculate timeout in ticks
    uint32_t timeout_limit = get_ticks_ms(_delay/2);
    uint32_t start_time = DWT_CYCCNT;

    // start polling
    while (1)
    {

        spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 1);

        // ready bit is bit0 of response byte 3
        if (avr_spi_rx[3] & 0x01)
            return true;

        if (DWT_CYCCNT - start_time > timeout_limit)
            break;

        delay_us(50);
    }

    return false;
}


uint8_t* avr_spi_transmit(uint8_t* tx)
{
    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    if (tx[0] == 0x30 && tx[2] < 3)
        avr_spi_sig |= avr_spi_rx[3] << (16 - (8 * tx[2]));

    if (tx[0] == 0x30 && tx[2] == 0)
        avr_device_isr_set_current_device(0);

    if (tx[0] == 0x30 && tx[2] == 2)
        avr_device_isr_set_current_device(avr_spi_sig);

    return avr_spi_rx;
}


void avr_spi_reset_ext_addr(void) {
    avr_spi_ext_addr = 0xFF;
}


void avr_spi_reset_sig(void) {
    avr_spi_sig = 0x00000000;
}

uint32_t avr_spi_get_sig(void) {
    return avr_spi_sig;
}


void avr_spi_check_ext_addr(uint32_t byte_addr)
{

    uint8_t ext_addr = (uint8_t)(byte_addr >> 17);
    if (ext_addr != avr_spi_ext_addr)
    {
        avr_spi_ext_addr = ext_addr;
        avr_spi_tx[0] = 0x4D; // Load Extended Address byte
        avr_spi_tx[1] = 0x00;
        avr_spi_tx[2] = ext_addr;
        avr_spi_tx[3] = 0x00;

        spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    }
}

uint8_t avr_spi_enableprog(void)
{
    avr_spi_tx[0] = 0xAC;
    avr_spi_tx[1] = 0x53;
    avr_spi_tx[2] = 0x00;
    avr_spi_tx[3] = 0x00;

    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    return avr_spi_rx[2];
}


uint8_t avr_spi_read_flash_byte(uint32_t byte_addr)
{
    
    uint16_t word = (uint16_t)(byte_addr >> 1); 
    avr_spi_tx[0] = (byte_addr & 1) ? 0x28 : 0x20;  
    avr_spi_tx[1] = (word >> 8) & 0xFF;
    avr_spi_tx[2] = word & 0xFF;
    avr_spi_tx[3] = 0x00;

    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    return avr_spi_rx[3];
}


void avr_spi_write_flash_byte(uint32_t byte_addr, uint8_t data)
{

    uint16_t word = (uint16_t)(byte_addr >> 1); 
    avr_spi_tx[0] = (byte_addr & 1) ? 0x48 : 0x40; 
    avr_spi_tx[1] = (word >> 8) & 0xFF;
    avr_spi_tx[2] = word & 0xFF;
    avr_spi_tx[3] = data;

    
    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    delay_us(3);
}


void avr_spi_flash_page_commit(uint32_t byte_addr)
{

    uint16_t word = (uint16_t)(byte_addr >> 1);
    avr_spi_tx[0] = 0x4C;
    avr_spi_tx[1] = (word >> 8) & 0xFF;
    avr_spi_tx[2] = word & 0xFF;
    avr_spi_tx[3] = 0x00;

    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    avr_spi_wait_ready(FLASH_DELAY);
    
}

uint8_t avr_spi_read_eeprom_byte(uint32_t byte_addr)
{
    avr_spi_tx[0] = 0xA0;  
    avr_spi_tx[1] = (byte_addr >> 8) & 0xFF;
    avr_spi_tx[2] = byte_addr & 0xFF;
    avr_spi_tx[3] = 0x00;

    
    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    return avr_spi_rx[3];
}

void avr_spi_write_eeprom_byte(uint32_t byte_addr, uint8_t data)
{
    avr_spi_tx[0] = 0xC0;
    avr_spi_tx[1] = (byte_addr >> 8) & 0xFF;
    avr_spi_tx[2] = byte_addr & 0xFF;
    avr_spi_tx[3] = data;

    
    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);
 
    delay_us(3);
    
}

void avr_spi_eeprom_page_commit(uint32_t byte_addr)
{
    avr_spi_tx[0] = 0xC2;
    avr_spi_tx[1] = (byte_addr >> 8) & 0xFF;
    avr_spi_tx[2] = byte_addr & 0xF8;
    avr_spi_tx[3] = 0x00;

    
    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    avr_spi_wait_ready(EEPROM_DELAY);
    
}



void avr_spi_chip_erase(void)
{
    avr_spi_tx[0] = 0xAC;
    avr_spi_tx[1] = 0x80;
    avr_spi_tx[2] = 0x00;
    avr_spi_tx[3] = 0x00;

    
    spi_tx_rx(avr_spi_tx, avr_spi_rx, 4, 5);

    delay_ms(50);

}

