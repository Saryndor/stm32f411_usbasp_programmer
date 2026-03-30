// usb_descriptors.c
#include "tusb.h"
#include "uid.h"
// #include "usb_descriptors.h"

// Optional: change these IDs for your project
#define USB_VID  0x16C0
#define USB_PID  0x05DC
#define USB_BCD  0x0100


// ---- Device descriptor (unchanged, keep your VID/PID/strings indices) ----
tusb_desc_device_t const desc_device = {
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0200,
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = USB_VID,
  .idProduct          = USB_PID,
  .bcdDevice          = USB_BCD,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
  return (uint8_t const *) &desc_device;
}


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





// ---- Configuration descriptor: CDC (ACM) + Vendor (bulk) ----
/*
 * FS max packet sizes implied by TinyUSB macros:
 * - CDC notification: 8 bytes
 * - CDC data IN/OUT:  64 bytes
 * - Vendor bulk IN/OUT: 64 bytes
 */
uint8_t const desc_configuration[] = {
  // config number, interface count, string index, total length, attributes, power (mA)
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // CDC ACM (one control + one data interface)
  // itf#,             str#,           ep_notif, ep_size,     ep_out,       ep_in,      ep_size
  TUD_CDC_DESCRIPTOR(  ITF_NUM_CDC,    4,        EPNUM_CDC_NOTIF, 8,  EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

  // Vendor (generic bulk in/out)
  // itf#,                str#, ep_out,          ep_in,           ep_size
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 5,   EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64),
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
  (void) index;
  return desc_configuration;
}

// ---- Strings ----
// index 0 is language (0x0409); keep your serial via mcu_uid_to_hex at index 3.
char const *string_desc_arr[] = {
  (const char[]){ 0x09, 0x04 }, // 0: English (0x0409)
  "Saryndor Dev.",              // 1: Manufacturer
  "STM32F411 usbasp",           // 2: Product
  "SerialFromUID",              // 3: overwritten by mcu_uid_to_hex at runtime
  "CDC Interface",              // 4: CDC string
  "USBasp (Vendor)",            // 5: Vendor string
};

// TU_VERIFY_STATIC(sizeof(desc_configuration) == CONFIG_TOTAL_LEN, "config length mismatch");

static uint16_t _desc_str[32];

// Build TinyUSB string descriptor into caller-provided buffer.
// - s:     zero-terminated ASCII (or UTF-8 in ASCII range)
// - desc:  buffer of 16-bit words
// - max16: capacity of 'desc' in 16-bit words
// Returns pointer to 'desc' (TinyUSB expects uint16_t const*).
static uint16_t const* make_string_desc(char const* s, uint16_t *desc, uint16_t max16)
{
  if (!s) s = "";

  // Number of UTF-16 code units available for payload (excluding header word)
  uint16_t cap_chars = (max16 > 1) ? (uint16_t)(max16 - 1) : 0;

  // Limit to buffer capacity
  uint16_t len = (uint16_t)strlen(s);
  if (len > cap_chars) len = cap_chars;

  // bLength (in bytes) and bDescriptorType in one 16-bit word:
  // upper byte = TUSB_DESC_STRING, lower byte = total length in bytes
  desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (uint8_t)(2 * len + 2));

  // ASCII -> UTF-16LE (zero high byte)
  for (uint16_t i = 0; i < len; i++) {
    desc[1 + i] = (uint8_t)s[i];
  }

  return desc;
}


uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;
  uint8_t chr_count;

  if ( index == 0 ) {
    _desc_str[1] = 0x0409;
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2+2);
    return _desc_str;
  }

  if ( index == 3 )  { // iSerial
	  static char serial[25]; // 24 hex + NUL
	  mcu_uid_to_hex(serial, sizeof(serial));
	  return make_string_desc(serial, _desc_str, 32);
	}

  if ( index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0]) ) return NULL;

  const char* str = string_desc_arr[index];

  // Cap at 31 UTF-16 code units
  chr_count = (uint8_t) strlen(str);
  if ( chr_count > 31 ) chr_count = 31;

  for (uint8_t i = 0; i < chr_count; i++) {
    _desc_str[1+i] = str[i];
  }

  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
  return _desc_str;
}



