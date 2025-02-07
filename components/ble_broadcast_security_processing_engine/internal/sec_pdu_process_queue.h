#ifndef SEC_PDU_PROCESS_QUEUE
#define SEC_PDU_PROCESS_QUEUE

#include <stdint.h>
#include <stddef.h>
#include "esp_gap_ble_api.h"

int enqueue_pdu_for_processing(uint8_t* data, size_t size, esp_bd_addr_t mac_address);

#endif