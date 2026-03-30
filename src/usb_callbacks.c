// usb_callbacks.c

#include <libopencm3/stm32/usart.h>
#include "tusb.h"
#include <stdbool.h>
#include "usb_descriptors.h"
#include "uart_dma.h" 
#include "usbasp_dispatch.h"


static volatile bool s_cdc_ready = false;

bool tud_cdc_is_ready(void) {
  // Optional helper if you want to query readiness elsewhere
  return s_cdc_ready && tud_cdc_connected();
}

// ---- Device lifecycle ----
void tud_mount_cb(void) {
  // Device is mounted by host
  s_cdc_ready = tud_cdc_connected();
}

void tud_umount_cb(void) {
  // Device is unplugged
  s_cdc_ready = false;
}

void tud_suspend_cb(bool remote_wakeup_en) {
  (void) remote_wakeup_en;
  // Bus suspended. You could enter low-power mode here.
}

void tud_resume_cb(void) {
  // Bus resumed
}

// ---- CDC ----
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
	(void)itf; (void) rts;
	// Host opened/closed the port. DTR ON typically means "terminal ready".
	s_cdc_ready = dtr && tud_cdc_connected();
}



void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p)
{
    (void) itf;
    
    // Map TinyUSB line coding to UART2 config
    // Only baudrate here for brevity
    
    usart_disable(USART2);
    usart_set_baudrate(USART2, p->bit_rate);
    usart_enable(USART2);
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                tusb_control_request_t const *request)
{

    switch (stage)
    {
        case CONTROL_STAGE_SETUP:
            return usbasp_dispatch_setup(rhport, request);

        case CONTROL_STAGE_ACK:
            return usbasp_dispatch_ack(rhport, request);

        default:
            return true;
    }
}