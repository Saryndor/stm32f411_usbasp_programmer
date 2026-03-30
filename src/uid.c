#include "uid.h"

#define UID_BASE (0x1FFF7A10UL) // STM32F411 UID base

void mcu_uid_to_hex(char *out, uint8_t out_len) {
  const uint32_t *uid = (const uint32_t *)UID_BASE;
  // 96-bit UID -> 24 hex chars + NUL = 25
  const char *hex = "0123456789ABCDEF";
  uint8_t i = 0;
  for (int w = 0; w < 3; ++w) {
    uint32_t v = uid[w];
    for (int b = 0; b < 8; ++b) {
      if (i+2 >= out_len) break;
      uint8_t nib = (v >> 28) & 0xF;
      out[i++] = hex[nib];
      v <<= 4;
    }
  }
  if (i < out_len) out[i] = '\0';
}
