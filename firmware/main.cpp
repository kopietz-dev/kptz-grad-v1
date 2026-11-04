#include "llcc68.h"
#include "pico/stdlib.h"
#include "static_vector.h"
#include "tusb.h"
#include <cstdint>
#include <cstdio>
#include <hardware/gpio.h>

#define TX_DONE 0xc1
#define RX_DONE 0xc2
#define TX_CMD 0xf1
#define RX_CMD 0xf2
#define BUF_CMD 0xf3
#define LED1_STATE 0xf4

#define BUF_SIZE 64

StaticVector<uint8_t, 5> cmdQueue;

uint8_t buffer[BUF_SIZE];
uint8_t bufferLen;

void write_cdc(const void *v, uint8_t n) {
  if (tud_cdc_connected() && tud_cdc_write_available() >= n) {
    tud_cdc_write(v, n);
    tud_cdc_write_flush();
  }
}

void tud_mount_cb(void) {}
void tud_umount_cb(void) {}
void tud_suspend_cb(bool) {}
void tud_resume_cb(void) {}

void tud_cdc_line_state_cb(uint8_t, bool, bool) {}
void tud_cdc_rx_cb(uint8_t itf) {
  bufferLen = tud_cdc_read(buffer, sizeof(buffer));
  if (bufferLen == 0)
    return;

  cmdQueue.push_back(TX_CMD);
}

void blink1() {
  gpio_put(PIN_LED1, 1);
  add_alarm_in_ms(
      200,
      [](alarm_id_t, void *) -> int64_t {
        gpio_put(PIN_LED1, 0);
        return 0;
      },
      nullptr, false);
}
void blink2() {
  gpio_put(PIN_LED2, 1);
  add_alarm_in_ms(
      200,
      [](alarm_id_t, void *) -> int64_t {
        gpio_put(PIN_LED2, 0);
        return 0;
      },
      nullptr, false);
}

void enable_tx() {
  clear_irq_status(IRQ_ALL);
  gpio_put(PIN_RXEN, 0);
  gpio_put(PIN_TXEN, 1);
  wait_for_llcc68();
  set_tx();
}
void enable_rx() {
  clear_irq_status(IRQ_ALL);
  gpio_put(PIN_RXEN, 1);
  gpio_put(PIN_TXEN, 0);
  wait_for_llcc68();
  set_rx(0);
}
void handleCmd(uint8_t cmd) {
  switch (cmd) {
  case TX_DONE:
    enable_rx();
    break;
  case RX_DONE:
    uint8_t payload_len, rx_addr;
    get_rx_buffer_status(&payload_len, &rx_addr);
    uint8_t rx_buffer[255];
    read_buffer(rx_buffer, payload_len);
    write_cdc(&rx_buffer, payload_len);
    enable_rx();

      blink1();

    break;
  case TX_CMD:
    write_buffer(buffer, bufferLen);

      blink2();
    enable_tx();
    break;
  }
}
void dio_cb(uint gpio, uint32_t events) {
  if (events)
    switch (gpio) {
    case PIN_DIO1:
      cmdQueue.push_back(RX_DONE);
      break;
    case PIN_DIO2:
      cmdQueue.push_back(TX_DONE);
      break;
    }
}
int main() {
  tusb_init();
  stdio_init_all();
  llcc68_init(433000000ULL, 14, 7, 125, BUF_SIZE);

  gpio_set_irq_enabled_with_callback(PIN_DIO1, GPIO_IRQ_EDGE_RISE, true,
                                     &dio_cb);
  gpio_set_irq_enabled_with_callback(PIN_DIO2, GPIO_IRQ_EDGE_RISE, true,
                                     &dio_cb);

blink1();
  blink2();

  enable_rx();
  while (1) {
    tud_task();

    while (cmdQueue.size() > 0) {
      handleCmd(cmdQueue.pop_back());
    }

    sleep_ms(1);
  }
}
