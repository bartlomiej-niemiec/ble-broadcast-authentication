#ifndef SEC_PDU_PROCESSING_H
#define SEC_PDU_PROCESSING_H

#include "esp_gap_ble_api.h"
#include "sec_payload_decrypted_observer.h"
#include "beacon_pdu_data.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_OBSERVERS 2
#define MAX_BLE_CONSUMERS 2

typedef struct{
    uint8_t data[MAX_GAP_DATA_LEN];
    size_t data_len;
    esp_bd_addr_t mac_address;
} ble_broadcast_pdu;

int start_up_sec_processing();

void register_payload_observer_cb(payload_decrypted_observer_cb observer_cb);

bool create_ble_broadcast_pdu_for_dispatcher(ble_broadcast_pdu* pdu, uint8_t *data, size_t size, esp_bd_addr_t mac_address);

void reset_processing();

#endif