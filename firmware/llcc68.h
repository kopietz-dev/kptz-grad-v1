#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <cstdint>

uint8_t dummy_tx[32] = {0};
uint8_t dummy_rx[32] = {0};

#define PIN_LED1 25
#define PIN_LED2 23

#define PIN_TXEN 19
#define PIN_RXEN 18

#define PIN_BUSY 17

#define PIN_DIO1 5
#define PIN_DIO2 6
#define PIN_DIO3 7

#define PIN_CS 9
#define PIN_SCK 10
#define PIN_MOSI 11
#define PIN_MISO 12

void llcc68gpio_init() {
  spi_init(spi1, 10000);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_put(PIN_CS, 1);

  gpio_init(PIN_TXEN);
  gpio_set_dir(PIN_TXEN, GPIO_OUT);

  gpio_init(PIN_RXEN);
  gpio_set_dir(PIN_RXEN, GPIO_OUT);

  gpio_init(PIN_LED1);
  gpio_set_dir(PIN_LED1, GPIO_OUT);

  gpio_init(PIN_LED2);
  gpio_set_dir(PIN_LED2, GPIO_OUT);

  gpio_init(PIN_BUSY);
  gpio_set_dir(PIN_BUSY, GPIO_IN);
}

void read_reg(uint16_t reg, uint8_t *rx, uint8_t n) {
  uint8_t cmd = 0x1D;
  uint8_t tx[3];

  tx[0] = (reg >> 8) & 0xFF;
  tx[1] = reg & 0xFF;
  tx[2] = 0x00;

  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, tx, dummy_rx, 3);
  spi_write_read_blocking(spi1, dummy_tx, rx, n);
  gpio_put(PIN_CS, 1);
}

void send_cmd(uint8_t cmd, void *v, uint8_t n) {
  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, (uint8_t *)v, dummy_rx, n);
  gpio_put(PIN_CS, 1);
}

void send_cmd(uint8_t cmd, uint8_t v) { send_cmd(cmd, &v, 1); }

void set_standby(uint8_t mode) { send_cmd(0x80, mode); }
void set_packet_type() { send_cmd(0x8A, 0x01); }

void set_rf_freq(uint32_t freq_hz) {
  uint32_t freq_reg = (uint32_t)((uint64_t)freq_hz * 33554432ULL / 32000000ULL);

  uint8_t data[4];
  data[0] = (freq_reg >> 24) & 0xFF;
  data[1] = (freq_reg >> 16) & 0xFF;
  data[2] = (freq_reg >> 8) & 0xFF;
  data[3] = freq_reg & 0xFF;

  send_cmd(0x86, data, 4);
}
void set_pa_config(uint8_t power_dbm) {
  uint8_t tx[4];
  switch (power_dbm) {

  case 17:
    tx[0] = 0x02;
    tx[1] = 0x03;
    tx[2] = 0x00;
    tx[3] = 0x01;
    break;
  case 20:
    tx[0] = 0x03;
    tx[1] = 0x05;
    tx[2] = 0x00;
    tx[3] = 0x01;
    break;
  case 22:
    tx[0] = 0x04;
    tx[1] = 0x07;
    tx[2] = 0x00;
    tx[3] = 0x01;
    break;
  case 14:
  default:
    tx[0] = 0x02;
    tx[1] = 0x02;
    tx[2] = 0x00;
    tx[3] = 0x01;
    break;
  }

  // uint8_t tx[4] = {0x02, 0x02, 0x00, 0x01}; // +14dBm

  send_cmd(0x95, tx, 4);
}
void set_tx_params() {
  uint8_t tx[2] = {0x16, 0x07};
  send_cmd(0x8E, tx, 2);
}
void set_buffer_base_address() {
  uint8_t tx[2] = {0x00, 0x00};
  send_cmd(0x8F, tx, 2);
}
void write_buffer(uint8_t *data, uint8_t n) {
  uint8_t cmd = 0x0E, offset = 0x00;

  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, &offset, dummy_rx, 1);
  spi_write_read_blocking(spi1, data, dummy_rx, n);
  gpio_put(PIN_CS, 1);
}

void read_buffer(uint8_t *data, uint8_t n) {
  uint8_t cmd = 0x1E, offset = 0x00;

  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, &offset, dummy_rx, 1);
  spi_write_read_blocking(spi1, dummy_tx, dummy_rx, 1);
  spi_write_read_blocking(spi1, dummy_tx, data, n);
  gpio_put(PIN_CS, 1);
}

void set_modulation_params(uint8_t sf, uint16_t bw_khz) {
  uint8_t bw_b = 0x00;
  switch (bw_khz) {
  case 500:
    bw_b = 0x06;
    break;
  case 250:
    bw_b = 0x05;
    break;
  default:
  case 125:
    bw_b = 0x04;
    break;
  }

  uint8_t ldro = 0x00;
  if (sf > 9 || bw_khz == 125)
    ldro = 0x01;

  uint8_t tx[4] = {sf, bw_b, 0x04, ldro};
  send_cmd(0x8B, tx, 4);
}

void set_packet_params(uint8_t dataLength) {
  uint8_t tx[6] = {0x00, 0x08, 0x00, dataLength, 0x01, 0x00};

  send_cmd(0x8C, tx, 6);
}

void set_tx() {
  uint8_t tx[3] = {0x00, 0x00, 0x00};
  send_cmd(0x83, tx, 3);
}

void calibration_image() {
  uint8_t tx[2] = {0x6B, 0x6F};
  send_cmd(0x98, tx, 2);
}

void get_device_error(uint8_t *rx) {
  uint8_t cmd = 0x17;

  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, dummy_tx, rx, 3);
  gpio_put(PIN_CS, 1);
}

void set_rx(uint32_t timeout_ms) {
  // timeout is in units of 15.625 μs
  // 0x000000 = no timeout (continuous RX)
  // 0xFFFFFF = single mode (RX until packet received)
  uint32_t timeout_val;
  if (timeout_ms == 0) {
    timeout_val = 0x000000;
  } else {
    timeout_val = (timeout_ms * 64);
  }

  uint8_t tx[3];
  tx[0] = (timeout_val >> 16) & 0xFF;
  tx[1] = (timeout_val >> 8) & 0xFF;
  tx[2] = timeout_val & 0xFF;
  send_cmd(0x82, tx, 3);
}

void get_irq_status(uint8_t *irq_status) {
  uint8_t cmd = 0x12;
  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, dummy_tx, dummy_rx, 1); // Status byte
  spi_write_read_blocking(spi1, dummy_tx, irq_status, 2);
  gpio_put(PIN_CS, 1);
}
#define IRQ_TX_DONE 0x0001
#define IRQ_RX_DONE 0x0002
#define IRQ_PREAMBLE_DETECTED 0x0004
#define IRQ_SYNC_WORD_VALID 0x0008
#define IRQ_HEADER_VALID 0x0010
#define IRQ_HEADER_ERROR 0x0020
#define IRQ_CRC_ERROR 0x0040
#define IRQ_CAD_DONE 0x0080
#define IRQ_CAD_DETECTED 0x0100
#define IRQ_TIMEOUT 0x0200
#define IRQ_ALL 0x03FF
void set_dio_irq_params(uint16_t irq_mask, uint16_t dio1_mask,
                        uint16_t dio2_mask, uint16_t dio3_mask) {
  uint8_t tx[8];
  tx[0] = (irq_mask >> 8) & 0xFF;
  tx[1] = irq_mask & 0xFF;
  tx[2] = (dio1_mask >> 8) & 0xFF;
  tx[3] = dio1_mask & 0xFF;
  tx[4] = (dio2_mask >> 8) & 0xFF;
  tx[5] = dio2_mask & 0xFF;
  tx[6] = (dio3_mask >> 8) & 0xFF;
  tx[7] = dio3_mask & 0xFF;
  send_cmd(0x08, tx, 8);
}

void get_rx_buffer_status(uint8_t *payload_length, uint8_t *rx_start_addr) {
  uint8_t cmd = 0x13;
  uint8_t rx[3];
  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, dummy_tx, rx, 3);
  gpio_put(PIN_CS, 1);

  *payload_length = rx[1];
  *rx_start_addr = rx[2];
}

void get_packet_status(int8_t *rssi, int8_t *snr) {
  uint8_t cmd = 0x14;
  uint8_t rx[4];
  gpio_put(PIN_CS, 0);
  spi_write_read_blocking(spi1, &cmd, dummy_rx, 1);
  spi_write_read_blocking(spi1, dummy_tx, rx, 4);
  gpio_put(PIN_CS, 1);

  *rssi = -rx[1] / 2; // RSSI in dBm
  *snr = rx[2] / 4;   // SNR in dB
}

void clear_irq_status(uint16_t irq_mask) {
  uint8_t tx[2];
  tx[0] = (irq_mask >> 8) & 0xFF;
  tx[1] = irq_mask & 0xFF;
  send_cmd(0x02, tx, 2);
}

void wait_for_llcc68() {
  while (gpio_get(PIN_BUSY))
    sleep_us(10);
}

void llcc68_init(uint32_t freq_hz, uint8_t power_dbm, uint8_t sf,
                 uint16_t bw_khz, uint8_t dataLength) {
  llcc68gpio_init();

  wait_for_llcc68();
  set_standby(0x00);
  wait_for_llcc68();
  set_standby(0x01);

  wait_for_llcc68();
  set_packet_type();

  wait_for_llcc68();
  set_rf_freq(freq_hz);

  wait_for_llcc68();
  calibration_image();

  wait_for_llcc68();
  set_pa_config(power_dbm);

  wait_for_llcc68();
  set_tx_params();

  wait_for_llcc68();
  set_buffer_base_address();

  wait_for_llcc68();
  set_modulation_params(sf, bw_khz);

  wait_for_llcc68();
  set_packet_params(dataLength);

  wait_for_llcc68();
  set_dio_irq_params(IRQ_TX_DONE | IRQ_RX_DONE | IRQ_TIMEOUT | IRQ_CRC_ERROR,
                     IRQ_RX_DONE, IRQ_TX_DONE, 0);

  wait_for_llcc68();
  clear_irq_status(IRQ_ALL);

  wait_for_llcc68();
  set_standby(0x00);
}

// lora params: preamble 4; SF=7, bandwidth=125khz, cr4_8,
