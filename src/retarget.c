// retarget.c -- route printf() to TinyUSB CDC
#include <sys/unistd.h>
#include <sys/errno.h>
#include "tusb.h"

#ifdef DEBUG

int _write(int fd, const void *buf, size_t len) {
  if (fd == 1 || fd == 2) {
    if (!tud_cdc_connected()) return (int)len;
    size_t n = 0;
    while (n < len) {
      uint32_t wr = tud_cdc_write(((uint8_t*)buf) + n, len - n);
      n += wr;
      if (tud_cdc_write_available() < 64) 
        tud_cdc_write_flush();
      //tud_cdc_write_flush();
      if (wr == 0) {
        tud_task();
      }
    }
    return (int)len;
  }
  return -1; // nicht unterstützt
}

#endif
