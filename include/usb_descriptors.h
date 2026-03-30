// usb_descriptors.h
#pragma once

// Interface numbers (must be contiguous from 0)
enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_VENDOR,
  ITF_NUM_TOTAL
};

// Unique endpoint numbers per direction (FS)
#define EPNUM_CDC_NOTIF   0x83   // IN  interrupt
#define EPNUM_CDC_OUT     0x01   // OUT bulk
#define EPNUM_CDC_IN      0x81   // IN  bulk

#define EPNUM_VENDOR_OUT  0x02   // OUT bulk
#define EPNUM_VENDOR_IN   0x82   // IN  bulk

// Total length of the single configuration
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_VENDOR_DESC_LEN)

