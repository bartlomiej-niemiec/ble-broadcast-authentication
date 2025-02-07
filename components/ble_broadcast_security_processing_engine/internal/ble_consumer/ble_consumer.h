#ifndef BLE_CONSUMER_H
#define BLE_CONSUMER_H

#include "esp_gap_ble_api.h"
#include "freertos/FreeRTOS.h"
#include "stdint.h"
#include "key_cache.h"
#include "key_reconstructor.h"
#include "beacon_pdu_data.h"

#define DEFERRED_QUEUE_SIZE 80
#define KEY_CACHE_SIZE 5

typedef struct {
    QueueHandle_t deferredQueue;
    uint8_t deferred_queue_count;
    key_reconstruction_cache *key_cache;
    uint8_t key_cache_size;
    int16_t recently_removed_key_id;
    bool process_deferred_q_request_pending;
} ble_consumer_context;


typedef struct {
    ble_consumer_context context;
    esp_bd_addr_t mac_address_arr;
    uint64_t last_pdu_timestamp;
    uint16_t last_pdu_key_id;
    uint8_t rollover;
    SemaphoreHandle_t xMutex;
} ble_consumer;


ble_consumer * create_ble_consumer(const uint8_t key_cache_size);

int create_ble_consumer_resources(ble_consumer *const p_ble_consumer, const uint8_t key_cache_size);

int destroy_ble_consumer(ble_consumer *p_ble_consumer);

int init_ble_consumer(ble_consumer *p_ble_consumer);

int reset_ble_consumer(ble_consumer * p_ble_consumer);

bool is_pdu_in_deferred_queue(ble_consumer *p_ble_consumer);

bool get_deferred_queue_item(ble_consumer* p_ble_consumer, beacon_pdu_data* pdu);

int add_to_deferred_queue(ble_consumer* p_ble_consumer, beacon_pdu_data* pdu);

void set_deferred_q_pending_processing(ble_consumer* p_ble_consumer, const bool request);

bool is_deferred_queue_request_pending(ble_consumer* p_ble_consumer);

#endif