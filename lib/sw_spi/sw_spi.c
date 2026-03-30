
#include <stdio.h>
#include <stdint.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#include "board_spi.h"
#include "delay.h"

static uint32_t sw_spi_delay = 6; // 375 khz
static uint32_t sw_delay_half_period = 0; // Delay in us for Bit-Banging

static uint32_t sw_spi_delay_table[9] = {
    1000, //    500 Hz
    500,  //    1 kHz
    250,  //    2 kHz
    125,  //    4 kHz
    62,   //    8 kHz
    31,   //   16 kHz
    15,   //   32 kHz
    5     //   93.75 khz
};

uint32_t sw_spi_get_delay(void)
{
  return sw_delay_half_period;
}

uint32_t sw_spi_set_delay(uint8_t sck_idx)
{
  if (sck_idx == 0)
    sw_delay_half_period = sw_spi_delay_table[7];
  else if (sck_idx < 9)
    sw_delay_half_period = sw_spi_delay_table[sck_idx - 1];
  else
    sw_delay_half_period = sw_spi_delay_table[7];

  return sw_delay_half_period;
}



// ---------------------------------------------------------------------------
// Activate SPI
// ---------------------------------------------------------------------------
void sw_spi_enable(void)
{


  // Enable peripheral clocks
  rcc_periph_clock_enable(SPI_RCC_GPIO);

  // Configure GPIO pins for SPI
  gpio_mode_setup(SPI_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPI_SCK_PIN | SPI_MOSI_PIN);
  gpio_mode_setup(SPI_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_MISO_PIN);

  // Set output pins (SCK, MOSI) to high speed push-pull
  gpio_set_output_options(SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_SCK_PIN | SPI_MOSI_PIN);

  delay_ms(1);
}

// ---------------------------------------------------------------------------
// Deactivate SPI
// ---------------------------------------------------------------------------
void sw_spi_disable(void)
{

  // Reset GPIO pins for SPI (PA5 = SCK, PA6 = MISO, PA7 = MOSI)
  gpio_mode_setup(SPI_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE, SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);

  delay_ms(1);
}

// --- Software SPI Helper ---
// Mode 0: CPOL=0, CPHA=0 (Sample on Rising, Setup on Falling)
static uint8_t sw_spi_transfer_byte(uint8_t data_out)
{
  uint8_t data_in = 0;

  // Ensure Clock starts LOW
  gpio_clear(SPI_PORT, SPI_SCK_PIN);

  for (int i = 7; i >= 0; i--)
  {
    // Setup Data (MOSI)
    if (data_out & (1 << i))
    {
      gpio_set(SPI_PORT, SPI_MOSI_PIN);
    }
    else
    {
      gpio_clear(SPI_PORT, SPI_MOSI_PIN);
    }

    delay_us(sw_delay_half_period);

    // Clock High (Sample MISO)
    gpio_set(SPI_PORT, SPI_SCK_PIN);

    // Read MISO
    if (gpio_get(SPI_PORT, SPI_MISO_PIN))
    {
      data_in |= (1 << i);
    }

    delay_us(sw_delay_half_period);

    // Clock Low
    gpio_clear(SPI_PORT, SPI_SCK_PIN);
  }
  return data_in;
}

// ---------------------------------------------------------------------------
// Transmit data via Software SPI
// ---------------------------------------------------------------------------
void sw_spi_transmit(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
  for (uint16_t i = 0; i < len; i++)
  {
    uint8_t val = tx ? tx[i] : 0xFF;
    uint8_t ret = sw_spi_transfer_byte(val);
    if (rx)
      rx[i] = ret;
  }
}