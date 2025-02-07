#include "beacon_pdu/beacon_pdu_data.h"
#include "ble_consumer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include <string.h>

// Creates a new BLE consumer
ble_consumer *create_ble_consumer(const uint8_t key_cache_size) {
    ble_consumer *p_ble_consumer = (ble_consumer *)malloc(sizeof(ble_consumer));
    if (!p_ble_consumer) {
        ESP_LOGE("BLE_CONSUMER", "Failed to allocate memory for BLE consumer");
        return NULL;
    }

    p_ble_consumer->context.key_cache_size = key_cache_size;
    if (create_key_cache(&(p_ble_consumer->context.key_cache), key_cache_size) != 0) {
        ESP_LOGE("BLE_CONSUMER", "Failed to create key cache");
        free(p_ble_consumer);
        return NULL;
    }

    p_ble_consumer->context.deferredQueue = xQueueCreate(DEFERRED_QUEUE_SIZE, sizeof(beacon_pdu_data));
    if (!p_ble_consumer->context.deferredQueue) {
        ESP_LOGE("BLE_CONSUMER", "Failed to create deferred queue");
        destroy_key_cache(p_ble_consumer->context.key_cache);
        free(p_ble_consumer);
        return NULL;
    }

    p_ble_consumer->xMutex = xSemaphoreCreateMutex();
    if (!p_ble_consumer->xMutex) {
        ESP_LOGE("BLE_CONSUMER", "Failed to create mutex");
        vQueueDelete(p_ble_consumer->context.deferredQueue);
        destroy_key_cache(p_ble_consumer->context.key_cache);
        free(p_ble_consumer);
        return NULL;
    }

    return p_ble_consumer;
}

int create_ble_consumer_resources(ble_consumer *const p_ble_consumer, const uint8_t key_cache_size)
{
    p_ble_consumer->context.key_cache_size = key_cache_size;
    if (create_key_cache(&(p_ble_consumer->context.key_cache), key_cache_size) != 0) {
        ESP_LOGE("BLE_CONSUMER", "Failed to create key cache");
        free(p_ble_consumer);
        return -1;
    }

    p_ble_consumer->context.deferredQueue = NULL;
    p_ble_consumer->context.deferredQueue = xQueueCreate(DEFERRED_QUEUE_SIZE, sizeof(beacon_pdu_data));
    if (!p_ble_consumer->context.deferredQueue) {
        ESP_LOGE("BLE_CONSUMER", "Failed to create deferred queue");
        destroy_key_cache(p_ble_consumer->context.key_cache);
        free(p_ble_consumer);
        return -1;
    }

    p_ble_consumer->xMutex = NULL;
    p_ble_consumer->xMutex = xSemaphoreCreateMutex();
    if (!p_ble_consumer->xMutex) {
        ESP_LOGE("BLE_CONSUMER", "Failed to create mutex");
        vQueueDelete(p_ble_consumer->context.deferredQueue);
        destroy_key_cache(p_ble_consumer->context.key_cache);
        free(p_ble_consumer);
        return -1;
    }
    
    return 0;
}

// Initializes BLE consumer context
int init_ble_consumer(ble_consumer *p_ble_consumer) {
    if (!p_ble_consumer) {
        return -1;
    }

    p_ble_consumer->context.process_deferred_q_request_pending = false;
    p_ble_consumer->last_pdu_timestamp = 0;
    p_ble_consumer->context.recently_removed_key_id = 0;
    p_ble_consumer->context.deferred_queue_count = 0;
    p_ble_consumer->rollover = 0;
    p_ble_consumer->last_pdu_key_id = 0;
    memset(p_ble_consumer->mac_address_arr, 0, sizeof(p_ble_consumer->mac_address_arr));

    return init_key_cache(p_ble_consumer->context.key_cache) == 0 ? 0 : -1;
}

// Resets BLE consumer
int reset_ble_consumer(ble_consumer *p_ble_consumer) {
    if (!p_ble_consumer) {
        return -1;
    }

    
    p_ble_consumer->context.process_deferred_q_request_pending = false;
    p_ble_consumer->last_pdu_timestamp = 0;
    p_ble_consumer->context.recently_removed_key_id = -1;
    p_ble_consumer->context.deferred_queue_count = 0;
    p_ble_consumer->rollover = 0;
    memset(&(p_ble_consumer->mac_address_arr), 0, sizeof(p_ble_consumer->mac_address_arr));
    clear_cache(p_ble_consumer->context.key_cache);
    xQueueReset(p_ble_consumer->context.deferredQueue);

    return 0;
}

// Destroys BLE consumer resources
int destroy_ble_consumer(ble_consumer *p_ble_consumer) {
    if (!p_ble_consumer) {
        return -1;
    }

    if (xSemaphoreTake(p_ble_consumer->xMutex, portMAX_DELAY) == pdTRUE) {
        if (p_ble_consumer->context.deferredQueue) {
            vQueueDelete(p_ble_consumer->context.deferredQueue);
        }

        if (p_ble_consumer->context.key_cache) {
            destroy_key_cache(p_ble_consumer->context.key_cache);
        }

        xSemaphoreGive(p_ble_consumer->xMutex);
    }

    vSemaphoreDelete(p_ble_consumer->xMutex);
    free(p_ble_consumer);
    return 0;
}

// Adds an item to the deferred queue
int add_to_deferred_queue(ble_consumer *p_ble_consumer, beacon_pdu_data *pdu) {
    if (!p_ble_consumer || !pdu) {
        return -1;
    }

    if (xQueueSend(p_ble_consumer->context.deferredQueue, (void *)pdu, 0) == pdTRUE) {
        xSemaphoreTake(p_ble_consumer->xMutex, portMAX_DELAY);
        p_ble_consumer->context.deferred_queue_count++;
        xSemaphoreGive(p_ble_consumer->xMutex);
        return 0;
    }

    return -1;
}

// Gets an item from the deferred queue
bool get_deferred_queue_item(ble_consumer *p_ble_consumer, beacon_pdu_data *pdu) {
    if (!p_ble_consumer || !pdu) {
        return false;
    }

    bool result = false;
    if (xSemaphoreTake(p_ble_consumer->xMutex, portMAX_DELAY) == pdTRUE)
    {
        if (xQueueReceive(p_ble_consumer->context.deferredQueue, pdu, 0) == pdTRUE) {
            if (p_ble_consumer->context.deferred_queue_count > 0) {
                p_ble_consumer->context.deferred_queue_count--;
            }
            result = true;
        }
        xSemaphoreGive(p_ble_consumer->xMutex);
    }
    else
    {
        ESP_LOGI("BLE_CONSUMER", "Failed to acquire semaphore while getting from deferred queue");
    }

    return result;
}

// Checks if there is a pending deferred queue processing request
bool is_deferred_queue_request_pending(ble_consumer *p_ble_consumer) {
    if (!p_ble_consumer) {
        return false;
    }
    bool is_pending = false;
    if (xSemaphoreTake(p_ble_consumer->xMutex, portMAX_DELAY) == pdTRUE)
    {
        is_pending = p_ble_consumer->context.process_deferred_q_request_pending;
        xSemaphoreGive(p_ble_consumer->xMutex);
    }

    return is_pending;
}

bool is_pdu_in_deferred_queue(ble_consumer *p_ble_consumer)
{
    if (!p_ble_consumer) {
        return false;
    }

    bool is_pdu_in_deferred_queue = false;
    if (xSemaphoreTake(p_ble_consumer->xMutex, portMAX_DELAY) == pdTRUE)
    {
        is_pdu_in_deferred_queue = p_ble_consumer->context.deferred_queue_count == 0 ? false : true;
        xSemaphoreGive(p_ble_consumer->xMutex);
    }
    else
    {
        ESP_LOGE("BLE_CONSUMER", "Failed to acquire mutex for setting deferred queue request.");
    }
    return is_pdu_in_deferred_queue;
}

void set_deferred_q_pending_processing(ble_consumer* p_ble_consumer, const bool request)
{
    if (!p_ble_consumer || !p_ble_consumer->xMutex) {
        ESP_LOGE("BLE_CONSUMER", "Invalid BLE consumer or mutex not initialized.");
        return;
    }

    if (xSemaphoreTake(p_ble_consumer->xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // Avoid redundant updates
        if (p_ble_consumer->context.process_deferred_q_request_pending != request) {
            p_ble_consumer->context.process_deferred_q_request_pending = request;
        }
        xSemaphoreGive(p_ble_consumer->xMutex);
    }
    else
    {
        ESP_LOGE("BLE_CONSUMER", "Failed to acquire mutex after timeout.");
    }
}


