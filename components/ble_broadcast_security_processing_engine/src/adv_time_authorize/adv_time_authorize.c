#include "sec_pdu_processing_scan_callback.h"
#include "sec_pdu_processing.h"
#include "adv_time_authorize.h"
#include "sec_pdu_process_queue.h"
#include "beacon_pdu_data.h"
#include "tasks_data.h"

#include "test.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include <stdatomic.h>

#define CONSUMER_PRIVATE_QUEUE_SIZE 50
#define EVENT_QUEUE_SIZE 10

#define MAX_PDU_PROCESS_PER_CONSUMER 6
#define NO_PDU_IN_QUEUE_FOR_PROCESS 6

#define MAX_BLOCK_TIME_TICKS pdMS_TO_TICKS(50)

#define ADV_AUTHORIZE_LOG "ADV_AUTHRORIZE"

#define EVENT_AUTHORIZE_PACKETS (1 << 1)

typedef struct {
    int64_t timestamp_us;
    uint8_t data[MAX_GAP_DATA_LEN];
    size_t size;
    uint16_t pdu_no;
    uint16_t key_id;
} scan_pdu;

typedef struct {
    esp_bd_addr_t consumer_addr;
    scan_pdu last_processed_pdu;
    QueueHandle_t privateQueue;
    uint16_t last_pdu_no;
    uint16_t last_pdu_key_id;
    bool active;
} consumer_authorization_structure;

typedef struct {
    consumer_authorization_structure consumers[MAX_BLE_CONSUMERS];
    SemaphoreHandle_t xMutex;
    TaskHandle_t xTaskHandle;
    EventGroupHandle_t eventGroup;
} adv_time_authorize_structure;

static adv_time_authorize_structure ao_control_structure;
static esp_bd_addr_t zero_address_init = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void adv_authorize_main(void *arg);
int get_consumer_index_for_addr(esp_bd_addr_t mac_address);
bool init_consumer_authorization_structure(consumer_authorization_structure *st);
uint32_t get_no_messages_in_queue(QueueHandle_t queue);
void process_authorization_for_consumer(uint8_t consumer_index);
void save_last_scanned_pdu(scan_pdu *prev_scanned_pdu, scan_pdu *pdu);
int get_tolerance_window_based_on_adv_interval(uint32_t adv_interval);

bool init_consumer_authorization_structure(consumer_authorization_structure *st)
{
    memset(st, 0, sizeof(consumer_authorization_structure));
    memcpy(st->consumer_addr, zero_address_init, sizeof(esp_bd_addr_t));
    st->privateQueue =  xQueueCreate(CONSUMER_PRIVATE_QUEUE_SIZE, sizeof(scan_pdu));
    if (st->privateQueue == NULL)
    {
        ESP_LOGI(ADV_AUTHORIZE_LOG, "Private Queue init failed");
        return false;
    }

    return true;
}

bool init_adv_time_authorize_object()
{
    static bool is_initialized_alread = false;

    if (is_initialized_alread == false)
    {
        for (int i = 0; i < MAX_BLE_CONSUMERS; i++)
        {
            bool result_consumer = init_consumer_authorization_structure(&ao_control_structure.consumers[i]);
            if (result_consumer == false)
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

        ao_control_structure.eventGroup = xEventGroupCreate();
        if (ao_control_structure.eventGroup == NULL)
        {
            ESP_LOGI(ADV_AUTHORIZE_LOG, "Event group alloc failed");
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

int get_consumer_index_for_addr(esp_bd_addr_t mac_address)
{
    int index = -1;
    int free_arr_index = -1;
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
        ao_control_structure.consumers[free_arr_index].active = true;
        index = free_arr_index;
    }

    return index;
}


uint32_t get_no_messages_in_queue(QueueHandle_t queue)
{
    return (uint32_t) uxQueueMessagesWaiting(queue);
}

void adv_authorize_main(void *arg)
{
    const uint32_t DELAY_TICKS = pdMS_TO_TICKS(20);
    while(1)
    {
        // Wait for events: either a new PDU or key reconstruction completion
        EventBits_t events = xEventGroupWaitBits(ao_control_structure.eventGroup,
                EVENT_AUTHORIZE_PACKETS,
                pdTRUE, pdFALSE, portMAX_DELAY);

        if (events &EVENT_AUTHORIZE_PACKETS)
        {
            for (int i = 0; i < MAX_BLE_CONSUMERS; i++)
            {
                if (ao_control_structure.consumers[i].active == true)
                {
                    process_authorization_for_consumer(i);
                    vTaskDelay(DELAY_TICKS);
                }
            }
        }
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
    scan_pdu * last_scan_pdu = NULL;
    if (ao_control_structure.consumers[consumer_index].last_processed_pdu.timestamp_us != 0)
    {
        start_index = 0;
        last_scan_pdu = &(ao_control_structure.consumers[consumer_index].last_processed_pdu);
    }
    else
    {
        last_scan_pdu = &pdus[0];
    }

    for (int i = start_index; i < batchCount; i++)
    {
        if (pdus[i].key_id == last_scan_pdu->key_id)
        {
            int64_t timestamp_diff_ms = ((pdus[i].timestamp_us - last_scan_pdu->timestamp_us) / 1000);
            uint32_t adv_time_for_key_id = get_adv_interval_from_key_id(pdus[i].key_id);
            int64_t timestamp_diff_from_pdus = (int64_t) ((pdus[i].pdu_no - last_scan_pdu->pdu_no) * adv_time_for_key_id);
            
            int difference_timestamps = (int) (timestamp_diff_from_pdus - timestamp_diff_ms);

            // ESP_LOGI(ADV_AUTHORIZE_LOG, "Elapsed time ms = %i", (int) timestamp_diff_ms);
            // ESP_LOGI(ADV_AUTHORIZE_LOG, "Approx elapsed time ms = %i", (int) timestamp_diff_from_pdus);
            // ESP_LOGI(ADV_AUTHORIZE_LOG, "CURR Packet PDU NO = %i, PREV PDU NO = %i", (int) pdus[i].pdu_no, (int) last_scan_pdu->pdu_no);
            // ESP_LOGI(ADV_AUTHORIZE_LOG, "Tim Diff = %i", difference_timestamps);

            const int PLUS_TOLERANCE_WINDOW_MS = get_tolerance_window_based_on_adv_interval(adv_time_for_key_id);
            const int MINUS_TOLERANCE_WINDOW_MS = PLUS_TOLERANCE_WINDOW_MS * -1;

            if ((difference_timestamps <= PLUS_TOLERANCE_WINDOW_MS) && (difference_timestamps >=  MINUS_TOLERANCE_WINDOW_MS))
            {
                enqueue_pdu_for_processing(last_scan_pdu->data, last_scan_pdu->size, ao_control_structure.consumers[consumer_index].consumer_addr);
            }
            else
            {
                test_log_adv_time_not_authorize(ao_control_structure.consumers[consumer_index].consumer_addr);
            }
        }

        last_scan_pdu = &pdus[i];
    }

    save_last_scanned_pdu(&(ao_control_structure.consumers[consumer_index].last_processed_pdu), last_scan_pdu);

}

int get_tolerance_window_based_on_adv_interval(uint32_t adv_interval)
{
    if (adv_interval >= 3000)
    {
        return adv_interval * 0.05;
    }
    else if (adv_interval >= 1000)
    {
        return adv_interval * 0.1;
    }
    else if (adv_interval > 500)
    {
        return adv_interval * 0.15;
    }
    else if (adv_interval > 200)
    {
        return adv_interval * 0.20;
    }
    else
    {
        return adv_interval * 0.30;
    }
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
            int consumer_index = get_consumer_index_for_addr(mac_address);
            if (consumer_index >= 0)
            {
                scan_pdu pdu;
                memcpy(pdu.data, data, data_size);
                pdu.timestamp_us = timestamp_us;
                memcpy(&(pdu.pdu_no), &(data[PDU_NO_OFFSET]), sizeof(uint16_t));
                memcpy(&(pdu.key_id), &(data[KEY_SESSION_OFFSET]), sizeof(uint16_t));
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

                        uint32_t no_messages_in_queue = get_no_messages_in_queue(ao_control_structure.consumers[consumer_index].privateQueue);
                        if (no_messages_in_queue >= NO_PDU_IN_QUEUE_FOR_PROCESS)
                        {
                            xEventGroupSetBits(ao_control_structure.eventGroup, EVENT_AUTHORIZE_PACKETS);
                        }
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

                    uint32_t no_messages_in_queue = get_no_messages_in_queue(ao_control_structure.consumers[consumer_index].privateQueue);
                    if (no_messages_in_queue >= NO_PDU_IN_QUEUE_FOR_PROCESS)
                    {
                        xEventGroupSetBits(ao_control_structure.eventGroup, EVENT_AUTHORIZE_PACKETS);
                    }
                }

            }
        }
    }
}


