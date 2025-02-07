#include "sec_pdu_processing_scan_callback.h"
#include "sec_pdu_processing.h"
#include "adv_time_authorize.h"
#include "sec_pdu_process_queue.h"
#include "beacon_pdu_data.h"
#include "tasks_data.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include <stdatomic.h>

#define CONSUMER_PRIVATE_QUEUE_SIZE 50
#define EVENT_QUEUE_SIZE 10

#define MAX_PDU_PROCESS_PER_CONSUMER 6
#define NO_PDU_IN_QUEUE_FOR_PROCESS 2

#define MAX_BLOCK_TIME_TICKS pdMS_TO_TICKS(50)

#define ADV_AUTHORIZE_LOG "ADV_AUTHRORIZE"

#define TOLERANCE_WINDOW_MS 20

typedef struct {
    int64_t timestamp_us;
    uint8_t data[MAX_GAP_DATA_LEN];
    size_t size;
    uint16_t pdu_no;
    uint16_t key_id;
} scan_pdu;

typedef struct {
    uint8_t consumer_index;
} authorize_event;

typedef struct {
    esp_bd_addr_t consumer_addr;
    scan_pdu last_processed_pdu;
    QueueHandle_t privateQueue;
    uint16_t last_pdu_no;
    uint16_t last_pdu_key_id;
} consumer_authorization_structure;

typedef struct {
    consumer_authorization_structure consumers[MAX_BLE_CONSUMERS];
    SemaphoreHandle_t xMutex;
    TaskHandle_t xTaskHandle;
} adv_time_authorize_structure;

static adv_time_authorize_structure ao_control_structure;
static esp_bd_addr_t zero_address_init = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void adv_authorize_main(void *arg);
uint8_t get_consumer_index_for_addr(esp_bd_addr_t mac_address);
bool init_consumer_authorization_structure(consumer_authorization_structure *st);
uint32_t get_no_messages_in_queue(QueueHandle_t queue);
void process_authorization_for_consumer(uint8_t consumer_index);
void save_last_scanned_pdu(scan_pdu *prev_scanned_pdu, scan_pdu *pdu);

bool init_consumer_authorization_structure(consumer_authorization_structure *st)
{
    memset(st, 0, sizeof(consumer_authorization_structure));
    memcpy(st->consumer_addr, zero_address_init, sizeof(esp_bd_addr_t));
    st->privateQueue =  xQueueCreate(CONSUMER_PRIVATE_QUEUE_SIZE, sizeof(scan_pdu));
    if (st->privateQueue == NULL)
    {
        ESP_LOGI(ADV_AUTHORIZE_LOG, "Private Queue init failed");
    }
}

bool init_adv_time_authorize_object()
{
    static is_initialized_alread = false;

    if (is_initialized_alread == false)
    {
        for (int i = 0; i < MAX_BLE_CONSUMERS; i++)
        {
            bool result_consumer = init_consumer_authorization_structure(&ao_control_structure.consumers[i]);
            if (result_consumer = false)
            {
                return is_initialized_alread;
            }
        }


        ao_control_structure.xMutex = xSemaphoreCreateMutex();
        if (ao_control_structure.xMutex == NULL)
        {
            ESP_LOGI(ADV_AUTHORIZE_LOG, "Mutex alloc Failed");
            return is_initialized_alread;
        }


        BaseType_t  taskCreateResult = xTaskCreatePinnedToCore(
            adv_authorize_main,
            tasksDataArr[ADV_TIME_AUTHORIZE_TASK].name, 
            tasksDataArr[ADV_TIME_AUTHORIZE_TASK].stackSize,
            NULL,
            tasksDataArr[ADV_TIME_AUTHORIZE_TASK].priority,
            &(ao_control_structure.xTaskHandle),
            tasksDataArr[ADV_TIME_AUTHORIZE_TASK].core
            );
    
        if (taskCreateResult != pdPASS)
        {
            ESP_LOGE(ADV_AUTHORIZE_LOG, "Task was not created successfully! :(");
            return is_initialized_alread;
        }
        else
        {
            ESP_LOGI(ADV_AUTHORIZE_LOG, "Task was created successfully! :)");
        }


        is_initialized_alread = true;
    }

    return is_initialized_alread;
}

uint8_t get_consumer_index_for_addr(esp_bd_addr_t mac_address)
{
    uint8_t index = -1;
    uint8_t free_arr_index = -1;
    if (xSemaphoreTake(ao_control_structure.xMutex, MAX_BLOCK_TIME_TICKS) == pdPASS)
    {
        for (int i = 0; i < MAX_BLE_CONSUMERS; i++)
        {
            if (memcmp(ao_control_structure.consumers[i].consumer_addr, mac_address, sizeof(esp_bd_addr_t)) == 0)
            {
                index = i;
                break;
            }
            else if (memcmp(ao_control_structure.consumers[i].consumer_addr, zero_address_init, sizeof(esp_bd_addr_t)) == 0)
            {
                free_arr_index = i;
            }
        }

        if (index == -1 && free_arr_index >= 0)
        {
            memcpy(ao_control_structure.consumers[free_arr_index].consumer_addr, mac_address, sizeof(esp_bd_addr_t));
            index = free_arr_index;
        }

        xSemaphoreGive(ao_control_structure.xMutex);
    }
    return index;
}


uint32_t get_no_messages_in_queue(QueueHandle_t queue)
{
    return (uint32_t) uxQueueMessagesWaiting(queue);
}

void adv_authorize_main(void *arg)
{
    const uint32_t DELAY_MS = 50;
    while(1)
    {
        for (int i = 0; i < MAX_BLE_CONSUMERS; i++)
        {
            uint32_t no_messages_in_queue = get_no_messages_in_queue(ao_control_structure.consumers[i].privateQueue);
            if (no_messages_in_queue >= NO_PDU_IN_QUEUE_FOR_PROCESS)
            {
                process_authorization_for_consumer(i);
                vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
    }

}

void process_authorization_for_consumer(const uint8_t consumer_index)
{
    scan_pdu pdus[MAX_PDU_PROCESS_PER_CONSUMER] = {0};
    int batchCount = 0;
    while (batchCount < MAX_PDU_PROCESS_PER_CONSUMER &&
        xQueueReceive(ao_control_structure.consumers[consumer_index].privateQueue, (void *)&pdus[batchCount], MAX_BLOCK_TIME_TICKS) == pdTRUE)
    {
        batchCount++;
    }

    int start_index = 1;
    scan_pdu prev_scanned_pdu;
    if (ao_control_structure.consumers[consumer_index].last_processed_pdu.timestamp_us != 0)
    {
        start_index = 0;
        save_last_scanned_pdu(&prev_scanned_pdu, &(ao_control_structure.consumers[consumer_index].last_processed_pdu));
    }
    else
    {
        save_last_scanned_pdu(&prev_scanned_pdu, &pdus[0]);
    }
    

    for (int i = 1; i < batchCount; i++)
    {
        if (pdus[i].key_id == prev_scanned_pdu.key_id)
        {
            int64_t timestamp_diff_ms = ((pdus[i].timestamp_us - prev_scanned_pdu.timestamp_us) / 1000);
            uint32_t adv_time_for_key_id = get_adv_interval_from_key_id(pdus[i].key_id);

            int64_t timestamp_diff_from_pdus = (int64_t) ((pdus[i].pdu_no - prev_scanned_pdu.pdu_no) * adv_time_for_key_id);
            
            int64_t difference_timestamps = timestamp_diff_from_pdus - timestamp_diff_ms;

            if ( difference_timestamps <= TOLERANCE_WINDOW_MS && difference_timestamps >  (-1 * TOLERANCE_WINDOW_MS))
            {
                ESP_LOGI(ADV_AUTHORIZE_LOG, "Timestamp difference: %i", (int) difference_timestamps);
                enqueue_pdu_for_processing(prev_scanned_pdu.data, prev_scanned_pdu.size, ao_control_structure.consumers[consumer_index].consumer_addr);
            }
        }
        save_last_scanned_pdu(&prev_scanned_pdu, &pdus[i]);
    }

    save_last_scanned_pdu(&(ao_control_structure.consumers[consumer_index].last_processed_pdu), &prev_scanned_pdu);

}

void save_last_scanned_pdu(scan_pdu *prev_scanned_pdu, scan_pdu *pdu)
{
    memcpy(prev_scanned_pdu, pdu, sizeof(scan_pdu));
}

void scan_complete_callback(int64_t timestamp_us, uint8_t *data, size_t data_size, esp_bd_addr_t mac_address)
{
    if (data != NULL)
    {
        if (is_pdu_in_beacon_pdu_format(data, data_size))
        {
            uint8_t consumer_index = get_consumer_index_for_addr(mac_address);
            if (consumer_index >= 0)
            {
                scan_pdu pdu;
                memcpy(pdu.data, data, data_size);
                pdu.timestamp_us = timestamp_us;
                memcpy(&(pdu.pdu_no), data[PDU_NO_OFFSET], sizeof(uint16_t));
                memcpy(&(pdu.key_id), data[KEY_SESSION_OFFSET], sizeof(uint16_t));
                pdu.size = data_size;
                uint16_t key_id = get_key_id_from_key_session_data(pdu.key_id);
                pdu.key_id = key_id;

                if (ao_control_structure.consumers->last_pdu_key_id == pdu.key_id)
                {
                    if (pdu.pdu_no > ao_control_structure.consumers->last_pdu_no)
                    {
                        BaseType_t queue_send_result =  xQueueSend(ao_control_structure.consumers[consumer_index].privateQueue, &pdu, MAX_BLOCK_TIME_TICKS);
                        if (queue_send_result != pdPASS)
                        {
                            ESP_LOGE(ADV_AUTHORIZE_LOG, "Failed add to queue! :(");
                        }
                        ao_control_structure.consumers->last_pdu_no = pdu.pdu_no;
                    }
                }
                else
                {
                    BaseType_t queue_send_result =  xQueueSend(ao_control_structure.consumers[consumer_index].privateQueue, &pdu, MAX_BLOCK_TIME_TICKS);
                    if (queue_send_result != pdPASS)
                    {
                        ESP_LOGE(ADV_AUTHORIZE_LOG, "Failed add to queue! :(");
                    }
                    ao_control_structure.consumers->last_pdu_no = pdu.pdu_no;
                    ao_control_structure.consumers->last_pdu_key_id = pdu.key_id;
                }

            }
        }
        else
        {
            if (is_pdu_from_expected_sender(mac_address) == true && is_test_pdu(data, data_size) != false)
            {
                test_log_bad_structure_packet(mac_address);
            }
        }
    }
}


