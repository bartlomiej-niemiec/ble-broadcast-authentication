#ifndef SEC_PDU_PROCESSING_QUEUE_H
#define SEC_PDU_PROCESSING_QUEUE_H

#include <stdint.h>
#include <stddef.h>
#include "esp_gap_ble_api.h"

void scan_complete_callback(int64_t timestamp_us, uint8_t *data, size_t data_size, esp_bd_addr_t mac_address);

#endif