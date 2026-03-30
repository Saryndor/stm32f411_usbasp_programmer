
#include <stdio.h>
#include <stdint.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "board_spi.h"
#include "hw_spi.h"
#include "delay.h"

static uint8_t  br_value = 7;  

// ---------------------------------------------------------------------------
// Activate SPI 
// ---------------------------------------------------------------------------
void hw_spi_enable(void)
{
   // Enable peripheral clocks
    rcc_periph_clock_enable(SPI_RCC_GPIO);
    rcc_periph_clock_enable(SPI_RCC_SPI);

    // Configure GPIO pins for SPI
    gpio_mode_setup(SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
    gpio_set_af(SPI_PORT, SPI_AF, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);
    
    // Set output pins (SCK, MOSI) to high speed push-pull
    gpio_set_output_options(SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_SCK_PIN | SPI_MOSI_PIN);

    // Reset SPI peripheral
    spi_disable(SPI_BASE);

    // Configure software slave management (NSS)
    spi_enable_software_slave_management(SPI_BASE);
    spi_set_nss_high(SPI_BASE);
    
    spi_init_master(SPI_BASE,
                    br_value << 3,         // Clock scaling
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,  // Clock polarity
                    SPI_CR1_CPHA_CLK_TRANSITION_1,    // Clock phase
                    SPI_CR1_DFF_8BIT,                 // Data frame format
                    SPI_CR1_MSBFIRST);                // Bit order
    
    

    // Enable SPI
    spi_enable(SPI_BASE);
    
    delay_ms(1);
}

// ---------------------------------------------------------------------------
// Deactivate SPI 
// ---------------------------------------------------------------------------
void hw_spi_disable(void)
{
    spi_disable(SPI_BASE);

    // Reset GPIO pins for SPI (PA5 = SCK, PA6 = MISO, PA7 = MOSI)
    gpio_mode_setup(SPI_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);

    delay_ms(1);
}



// ---------------------------------------------------------------------------
// Set SPI baudrate value 
// ---------------------------------------------------------------------------

/*
    sck_idx  0:   default  (hw_spi; 187.5 Mhz)
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

/* --- SPI2 Clock Divider (fPCLK = 48 MHz) ---

    000 (0): fPCLK/2    ->  24.0 MHz
    001 (1): fPCLK/4    ->  12.0 MHz
    010 (2): fPCLK/8    ->   6.0 MHz
    011 (3): fPCLK/16   ->   3.0 MHz
    100 (4): fPCLK/32   ->   1.5 MHz
    101 (5): fPCLK/64   -> 750.0 kHz
    110 (6): fPCLK/128  -> 375.0 kHz
    111 (7): fPCLK/256  -> 187.5 kHz
*/


uint8_t hw_spi_set_br_value(uint8_t sck_idx) {
    if ( sck_idx < 9 )
        br_value = 7;
    else if ( sck_idx < 14 )
        br_value = 16 - sck_idx;
    else
        br_value = 7;

    // spi_set_baudrate_prescaler(SPI_BASE, br_value);

    return br_value;    
}

// ---------------------------------------------------------------------------
// Get SPI baudrate value 
// ---------------------------------------------------------------------------
uint8_t hw_spi_get_br_value(void){
    return br_value;
}

// ---------------------------------------------------------------------------
// Recover SPI from a stuck state (Timeout)
// ---------------------------------------------------------------------------
void hw_spi_recover(void)
{
    // 1. Disable SPI to reset the internal state machine
    spi_disable(SPI_BASE);
    
    // 2. Dummy read the Data Register to clear the RXNE flag 
    // and flush any garbage data out of the receive buffer.
    volatile uint16_t dummy = SPI_DR(SPI_BASE);
    (void)dummy; // Prevent compiler warning for unused variable
    
    // 3. Re-enable SPI
    spi_enable(SPI_BASE);
}

// ---------------------------------------------------------------------------
// Send data over SPI 
// ---------------------------------------------------------------------------
bool hw_spi_transmit(const uint8_t *tx, uint8_t *rx, uint32_t len, uint32_t t)
{

    // 2. Dummy read the Data Register to clear the RXNE flag 
    // and flush any garbage data out of the receive buffer.
    SPI_DR(SPI_BASE);

    uint32_t timeout_limit = get_ticks_ms(t);

    for (uint16_t i = 0; i < len; i++)
    {
        uint32_t start_time = DWT_CYCCNT;

        // 1. Wait for TX Ready
        while (!(SPI_SR(SPI_BASE) & SPI_SR_TXE))
        {
            if ((DWT_CYCCNT - start_time) > timeout_limit)
            {
                // TIMEOUT DETECTED!
                // The hardware might be stuck. Reset the SPI block to recover.
                hw_spi_recover(); // Re-Init hardware to clear stuck state
                return false;
            }
        }

        uint8_t data_out = tx ? tx[i] : 0xFF;
        spi_write(SPI_BASE, data_out);

        // 3. Wait for RX Ready
        start_time = DWT_CYCCNT;
        while (!(SPI_SR(SPI_BASE) & SPI_SR_RXNE))
        {
            if ((DWT_CYCCNT - start_time) > timeout_limit)
            {
                // TIMEOUT DETECTED!
                // Target didn't respond or clock failure. Reset SPI block.
                hw_spi_recover(); // Re-Init hardware
                return false;
            }
        }

        uint8_t data_in = (uint8_t)(spi_read(SPI_BASE) & 0xFF);
        if (rx)
            rx[i] = data_in;
    }
    return true;
}
