#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/sync.h>

#include "tusb.h"
#include "delay.h" // Assuming this provides get_ticks_ms()
#include "uart_dma.h"

// ---------------------------------------------------------------------------
// Hardware Definitions (STM32F411 specific)
// USART2 is on APB1, DMA1
// USART2_RX = DMA1, Stream 5, Channel 4
// USART2_TX = DMA1, Stream 6, Channel 4
// ---------------------------------------------------------------------------
#define UART_PERIPH       USART2
#define DMA_PERIPH        DMA1
#define DMA_RX_STREAM     DMA_STREAM5
#define DMA_RX_CHANNEL    DMA_SxCR_CHSEL_4
#define DMA_TX_STREAM     DMA_STREAM6
#define DMA_TX_CHANNEL    DMA_SxCR_CHSEL_4

// ---------------------------------------------------------------------------
// RX Ring Buffer (DMA writes, CPU reads)
// ---------------------------------------------------------------------------
#define UART2_DMA_RX_SIZE   2048u
static uint8_t  uart2_dma_rx[UART2_DMA_RX_SIZE];
static volatile uint32_t uart2_rx_tail = 0;
static uint32_t last_flush_tick = 0;
static bool wrote_since_last = false;

// ---------------------------------------------------------------------------
// TX Ring Buffer (CPU writes, DMA reads)
// ---------------------------------------------------------------------------
#define UART2_DMA_TX_SIZE  2048u  // power-of-2
static uint8_t  uart2_dma_tx[UART2_DMA_TX_SIZE] __attribute__((aligned(4)));
static volatile uint32_t uart2_tx_head = 0;
static volatile uint32_t uart2_tx_tail = 0;
static volatile uint32_t uart2_tx_dma_len = 0;
static volatile uint8_t  uart2_tx_dma_active = 0;

// ===========================================================================
// INITIALIZATION
// ===========================================================================

void uart2_dma_init(void)
{
    // 1. Enable clocks
    rcc_periph_clock_enable(RCC_GPIOA); 
    rcc_periph_clock_enable(RCC_USART2);
    rcc_periph_clock_enable(RCC_DMA1);

    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);

    // 2. Configure USART2 (115200 8N1 default)
    usart_set_baudrate(UART_PERIPH, 115200);
    usart_set_databits(UART_PERIPH, 8);
    usart_set_stopbits(UART_PERIPH, USART_STOPBITS_1);
    usart_set_mode(UART_PERIPH, USART_MODE_TX_RX);
    usart_set_parity(UART_PERIPH, USART_PARITY_NONE);
    usart_set_flow_control(UART_PERIPH, USART_FLOWCONTROL_NONE);

    // 3. Configure DMA for RX (Circular Mode)
    dma_stream_reset(DMA_PERIPH, DMA_RX_STREAM);
    dma_set_peripheral_address(DMA_PERIPH, DMA_RX_STREAM, (uint32_t)&USART_DR(UART_PERIPH));
    dma_set_memory_address(DMA_PERIPH, DMA_RX_STREAM, (uint32_t)uart2_dma_rx);
    dma_set_number_of_data(DMA_PERIPH, DMA_RX_STREAM, UART2_DMA_RX_SIZE);
    dma_channel_select(DMA_PERIPH, DMA_RX_STREAM, DMA_RX_CHANNEL);
    dma_set_transfer_mode(DMA_PERIPH, DMA_RX_STREAM, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_set_memory_size(DMA_PERIPH, DMA_RX_STREAM, DMA_SxCR_MSIZE_8BIT);
    dma_set_peripheral_size(DMA_PERIPH, DMA_RX_STREAM, DMA_SxCR_PSIZE_8BIT);
    dma_enable_memory_increment_mode(DMA_PERIPH, DMA_RX_STREAM);
    dma_enable_circular_mode(DMA_PERIPH, DMA_RX_STREAM);
    
    // 4. Configure DMA for TX (Normal Mode, fired manually)
    dma_stream_reset(DMA_PERIPH, DMA_TX_STREAM);
    dma_set_peripheral_address(DMA_PERIPH, DMA_TX_STREAM, (uint32_t)&USART_DR(UART_PERIPH));
    dma_channel_select(DMA_PERIPH, DMA_TX_STREAM, DMA_TX_CHANNEL);
    dma_set_transfer_mode(DMA_PERIPH, DMA_TX_STREAM, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
    dma_set_memory_size(DMA_PERIPH, DMA_TX_STREAM, DMA_SxCR_MSIZE_8BIT);
    dma_set_peripheral_size(DMA_PERIPH, DMA_TX_STREAM, DMA_SxCR_PSIZE_8BIT);
    dma_enable_memory_increment_mode(DMA_PERIPH, DMA_TX_STREAM);
    dma_enable_transfer_complete_interrupt(DMA_PERIPH, DMA_TX_STREAM);

    // 5. Enable DMA Interrupt for TX in NVIC
    nvic_enable_irq(NVIC_DMA1_STREAM6_IRQ);

    // 6. Tell USART to use DMA
    usart_enable_rx_dma(UART_PERIPH);
    usart_enable_tx_dma(UART_PERIPH);

    // 7. Enable USART
    usart_enable(UART_PERIPH);
}

void uart2_dma_start(void)
{
    // Start continuous RX DMA
    dma_enable_stream(DMA_PERIPH, DMA_RX_STREAM);
    
    // Note: IDLE interrupt is not strictly needed for the cyclic buffer approach 
    // since we poll the DMA counter in drain_to_cdc0. But if you want to use it, 
    // you would enable it here and catch it in usart2_isr().
}

// ===========================================================================
// RX ROUTINES (UART -> CDC)
// ===========================================================================

static inline uint32_t uart2_dma_head(void)
{
    // Current write index of DMA (circular): head = N - NDTR
    uint32_t ndtr = dma_get_number_of_data(DMA_PERIPH, DMA_RX_STREAM);
    return (UART2_DMA_RX_SIZE - ndtr) & (UART2_DMA_RX_SIZE - 1);
}

void uart2_drain_to_cdc0(void)
{
    if (!tud_cdc_connected()) return;

    uint32_t head = uart2_dma_head();
    bool newly_wrote = false;

    while (uart2_rx_tail != head) {
        uint32_t chunk = (head >= uart2_rx_tail)
                       ? (head - uart2_rx_tail)
                       : (UART2_DMA_RX_SIZE - uart2_rx_tail);

        uint32_t space = tud_cdc_write_available();
        if (space == 0) break;

        uint32_t n = (chunk < space) ? chunk : space;
        uint32_t wrote = tud_cdc_write(uart2_dma_rx + uart2_rx_tail, n);
        if (wrote == 0) break;

        uart2_rx_tail = (uart2_rx_tail + wrote) & (UART2_DMA_RX_SIZE - 1);
        newly_wrote = true;
    }

    // Capture the exact start time of a new data burst
    if (newly_wrote && !wrote_since_last) {
        wrote_since_last = true;
        last_flush_tick = DWT_CYCCNT;
    }

    if (tud_cdc_write_available() < 64) {
        tud_cdc_write_flush();
        wrote_since_last = false;
        return;
    }

    // Check if 2ms (converted to ticks) have elapsed since the burst started
    if (wrote_since_last && ((DWT_CYCCNT - last_flush_tick) >= get_ticks_ms(2))) { 
        tud_cdc_write_flush();
        wrote_since_last = false;
    }
}

// ===========================================================================
// TX ROUTINES (CDC -> UART)
// ===========================================================================

static inline uint32_t uart2_tx_count(void) {
    return (uart2_tx_head - uart2_tx_tail) & (UART2_DMA_TX_SIZE - 1);
}

static inline uint32_t uart2_tx_space(void) {
    return (UART2_DMA_TX_SIZE - 1) - uart2_tx_count();
}

static uint32_t uart2_tx_ring_push(const uint8_t *src, uint32_t len)
{
    uint32_t space = uart2_tx_space();
    if (len > space) len = space;
    if (!len) return 0;

    uint32_t head = uart2_tx_head & (UART2_DMA_TX_SIZE - 1);
    uint32_t first = UART2_DMA_TX_SIZE - head;
    
    if (first > len) first = len;
    memcpy(&uart2_dma_tx[head], src, first);
    if (len > first) memcpy(&uart2_dma_tx[0], src + first, len - first);

    __dmb(); // Memory barrier (sync.h)
    uart2_tx_head = (uart2_tx_head + len) & (UART2_DMA_TX_SIZE - 1);
    return len;
}

static void uart2_tx_kick_dma(void)
{
    if (uart2_tx_dma_active) return;

    uint32_t head = uart2_tx_head & (UART2_DMA_TX_SIZE - 1);
    uint32_t tail = uart2_tx_tail & (UART2_DMA_TX_SIZE - 1);
    uint32_t avail = uart2_tx_count();
    
    if (avail == 0) return;

    uint32_t chunk = (head >= tail) ? (head - tail) : (UART2_DMA_TX_SIZE - tail);

    uart2_tx_dma_len   = chunk;
    uart2_tx_dma_active = 1;

    // Start DMA from ring tail
    dma_set_memory_address(DMA_PERIPH, DMA_TX_STREAM, (uint32_t)&uart2_dma_tx[tail]);
    dma_set_number_of_data(DMA_PERIPH, DMA_TX_STREAM, chunk);
    dma_enable_stream(DMA_PERIPH, DMA_TX_STREAM);
}

void cdc0_drain_to_uart2(void)
{
    if (!tud_cdc_connected()) return;

    uint8_t buf[256];

    while (1) {
        uint32_t avail = tud_cdc_available();
        if (avail == 0) break;

        if (avail > sizeof(buf)) avail = sizeof(buf);

        uint32_t space = uart2_tx_space();
        if (space == 0) break;

        uint32_t to_read = (avail < space) ? avail : space;

        uint32_t n = tud_cdc_read(buf, to_read);
        if (n == 0) break;

        (void) uart2_tx_ring_push(buf, n);
    }

    uart2_tx_kick_dma();
}

// ---------------------------------------------------------------------------
// Interrupt Handlers
// ---------------------------------------------------------------------------

// Replaces HAL_UART_TxCpltCallback
void dma1_stream6_isr(void)
{
    // Check if Transfer Complete Interrupt Flag is set
    if (dma_get_interrupt_flag(DMA_PERIPH, DMA_TX_STREAM, DMA_TCIF)) {
        // Clear the flag
        dma_clear_interrupt_flags(DMA_PERIPH, DMA_TX_STREAM, DMA_TCIF);
        
        // Advance tail and mark as inactive
        uart2_tx_tail = (uart2_tx_tail + uart2_tx_dma_len) & (UART2_DMA_TX_SIZE - 1);
        uart2_tx_dma_active = 0;
        uart2_tx_dma_len = 0;

        // Immediately try to send the next chunk if queued
        uart2_tx_kick_dma();
    }
}