#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#include "tusb.h"
#include "hw_spi.h"
#include "avr_spi.h"
#include "uart_dma.h"
#include "delay.h"

// Define LED pin (typically PC13 on STM32F411 BlackPill)
#define LED_PORT GPIOC
#define LED_PIN  GPIO13

// Provide SystemCoreClock for TinyUSB's DWC2 driver
uint32_t SystemCoreClock = 96000000;

// ---------------------------------------------------------------------------
// Interrupt Handler for USB OTG Full Speed
// ---------------------------------------------------------------------------
void otg_fs_isr(void)
{
    // Forward the interrupt to the TinyUSB stack
    tud_int_handler(0);
}

// ---------------------------------------------------------------------------
// Hardware Initialization
// ---------------------------------------------------------------------------
static void system_clock_setup(void)
{
    // Configure system clock for 96MHz using 25MHz external crystal (HSE).
    // This automatically sets up the PLL to output exactly 48MHz for the USB peripheral.
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_96MHZ]);
	SystemCoreClock = rcc_ahb_frequency;
}

static void board_pins_setup(void)
{
    // Enable clock for LED port
    rcc_periph_clock_enable(RCC_GPIOC);
    
    // Configure LED pin as output
    gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
    gpio_clear(LED_PORT, LED_PIN); // Set low (usually turns LED ON on BlackPill)

    avr_spi_oe_pin_init();
    avr_spi_miso_pwr_pin_init();
}

static void usb_setup(void)
{
    // Enable GPIOA clock for USB pins
    rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_OTGFS);
    
    // Force USB re-enumeration
    // Pull D+ (PA12) low for 80ms to tell the host we disconnected
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
    gpio_clear(GPIOA, GPIO12);
    delay_ms(80);
    
    // Configure PA11 (USB_DM) and PA12 (USB_DP) for USB OTG FS (Alternate Function 10)
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

	// Important: Enable USB Interrupt in NVIC
    // Without this, the stack stays silent
    nvic_enable_irq(NVIC_OTG_FS_IRQ);
}

// ---------------------------------------------------------------------------
// Main Application
// ---------------------------------------------------------------------------
int main(void)
{

    // Disable buffering for stdout/stderr (useful for printf via CDC later)
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // 1. Initialize core hardware
    system_clock_setup();
    delay_init(); // Assumes this sets up DWT or SysTick internally
    
    // 2. Initialize IOs and peripherals
    board_pins_setup();
    usb_setup();
    avr_spi_reset_pin_init();
    hw_spi_disable(); // Put SPI into default inactive state
    uart2_dma_init();

    // 3. Initialize External Libraries and Subsystems
    tusb_init();
    uart2_dma_start();

    // SystemCoreClock equivalent in libopencm3 is rcc_ahb_frequency
    printf("System Clock: %lu MHz\n", rcc_ahb_frequency / 1000000);

    // 4. Main Loop
    while (1)
    {
        // TinyUSB device task (polling, no RTOS)
        tud_task(); 
        
        // Handle UART <-> USB CDC routing
        uart2_drain_to_cdc0();
        cdc0_drain_to_uart2();
    }

    return 0;
}