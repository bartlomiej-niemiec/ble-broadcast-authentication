#include <stddef.h>
#include <string.h>

#include "crypto/crypto.h"
#include "key_management.h"
#include "key_reconstructor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "test.h"

#include "tasks_data.h"

#define MAX_ELEMENTS_IN_QUEUE 15

#define QUEUE_TIMEOUT_MS 20

#define MAX_KEY_PROCESSES_AT_ONCE 20

// Event group flags
#define EVENT_NEW_KEY_FARGMENT_IN_QUEUE (1 << 0)

typedef struct{
    uint16_t key_id;    
    uint8_t key_fragment_no;
    uint8_t encrypted_key_fragment[KEY_FRAGMENT_SIZE];
    uint8_t key_hmac[HMAC_SIZE];
    uint8_t xor_seed;
    esp_bd_addr_t consumer_mac_address;
} __attribute__((aligned(4))) reconstructor_queue_element;

static const char* REC_LOG_GROUP = "RECONSTRUCTION TASK";

typedef struct {
    TaskHandle_t xRecontructionKeyTask;
    QueueHandle_t xQueueKeyReconstruction;
    EventGroupHandle_t eventGroup;
    bool is_reconstructor_resources_init;
    key_reconstruction_complete_cb key_rec_cb;
    key_reconstruction_collection * key_collection;
} key_reconstructor_control;

static key_reconstructor_control st_reconstructor_control = {
    .xRecontructionKeyTask = NULL,
    .xQueueKeyReconstruction = NULL,
    .eventGroup = NULL,
    .is_reconstructor_resources_init = false,
    .key_rec_cb = NULL,
    .key_collection = NULL
};

void process_and_store_key_fragment(reconstructor_queue_element * q_element);
bool init_reconstructor_resources(const uint8_t key_reconstruction_collection_size);
void handle_event_new_key_fragment_in_queue();

void reconstructor_main(void *arg)
{
    ESP_LOGI(REC_LOG_GROUP, "Starting up key reconstructor task");
    while (1)
    {
         // Wait for events: either a new PDU or key reconstruction completion
        EventBits_t events = xEventGroupWaitBits(st_reconstructor_control.eventGroup,
                                                 EVENT_NEW_KEY_FARGMENT_IN_QUEUE,
                                                 pdTRUE, pdFALSE, portMAX_DELAY);

        if (events & EVENT_NEW_KEY_FARGMENT_IN_QUEUE) {
            handle_event_new_key_fragment_in_queue();
        }

    }
}

void handle_event_new_key_fragment_in_queue()
{
    reconstructor_queue_element keyFragmentBatch[MAX_KEY_PROCESSES_AT_ONCE];
    int counter = 0;
    while (counter < MAX_KEY_PROCESSES_AT_ONCE && xQueueReceive(st_reconstructor_control.xQueueKeyReconstruction, ( void * ) &keyFragmentBatch[counter], QUEUE_TIMEOUT_MS / portTICK_PERIOD_MS) == pdTRUE)
    {
        counter++;
    }


    for (int i = 0; i < counter; i++)
    {
        if (is_key_in_collection(st_reconstructor_control.key_collection, keyFragmentBatch[i].consumer_mac_address, keyFragmentBatch[i].key_id) == false)
        {
            add_new_key_to_collection(st_reconstructor_control.key_collection, keyFragmentBatch[i].consumer_mac_address, keyFragmentBatch[i].key_id);
            test_log_key_reconstruction_start(keyFragmentBatch[i].consumer_mac_address, keyFragmentBatch[i].key_id);
        }

        if (is_key_fragment_decrypted(st_reconstructor_control.key_collection, keyFragmentBatch[i].consumer_mac_address, keyFragmentBatch[i].key_id, keyFragmentBatch[i].key_fragment_no) == false)
        {
            process_and_store_key_fragment(&keyFragmentBatch[i]);
        }
        else
        {
            test_log_packet_received_key_fragment_already_decoded(keyFragmentBatch[i].consumer_mac_address);
        }

        if (is_key_available(st_reconstructor_control.key_collection, keyFragmentBatch[i].consumer_mac_address, keyFragmentBatch[i].key_id) == true)
        {
            key_128b reconstructed_key = {0};
            reconstruct_key_from_key_fragments(st_reconstructor_control.key_collection, &reconstructed_key, keyFragmentBatch[i].consumer_mac_address, keyFragmentBatch[i].key_id);
            if (st_reconstructor_control.key_rec_cb != NULL)
            {
                st_reconstructor_control.key_rec_cb(keyFragmentBatch[i].key_id, &reconstructed_key, keyFragmentBatch[i].consumer_mac_address);
                remove_key_from_collection(st_reconstructor_control.key_collection, keyFragmentBatch[i].consumer_mac_address, keyFragmentBatch[i].key_id);
            }
        }
    }




}


int start_up_key_reconstructor(const uint8_t max_key_reconstrunction_count) {

    int status = 0;
    st_reconstructor_control.is_reconstructor_resources_init = init_reconstructor_resources(max_key_reconstrunction_count);

    if (st_reconstructor_control.is_reconstructor_resources_init == true)
    {
        if (st_reconstructor_control.xRecontructionKeyTask == NULL)
        {
            BaseType_t  taskCreateResult = xTaskCreatePinnedToCore(
                reconstructor_main,
                tasksDataArr[KEY_RECONSTRUCTION_TASK].name, 
                tasksDataArr[KEY_RECONSTRUCTION_TASK].stackSize,
                NULL,
                tasksDataArr[KEY_RECONSTRUCTION_TASK].priority,
                &st_reconstructor_control.xRecontructionKeyTask,
                tasksDataArr[KEY_RECONSTRUCTION_TASK].core
                );
        
            if (taskCreateResult != pdPASS) {
                status = -2;
                ESP_LOGE(REC_LOG_GROUP, "Task was not created successfully! :(");
                // Clean up queue and mutex to avoid leaks
                vQueueDelete(st_reconstructor_control.xQueueKeyReconstruction);
                st_reconstructor_control.is_reconstructor_resources_init = false;
            } else {
                ESP_LOGI(REC_LOG_GROUP, "Task was created successfully! :)");
            }
        
        }
    }
    else
    {
        status = -1;
    }

    return status;
}

void register_callback_to_key_reconstruction(key_reconstruction_complete_cb cb)
{
    ESP_LOGI(REC_LOG_GROUP, "Callback has been registered");
    st_reconstructor_control.key_rec_cb = cb;
}

bool init_reconstructor_resources(const uint8_t key_reconstruction_collection_size)
{
    st_reconstructor_control.eventGroup = xEventGroupCreate();

    if(st_reconstructor_control.eventGroup == NULL)
    {
        return false;
    }

    st_reconstructor_control.xQueueKeyReconstruction = xQueueCreate(MAX_ELEMENTS_IN_QUEUE, sizeof(reconstructor_queue_element));

    if (st_reconstructor_control.xQueueKeyReconstruction == NULL)
    {
        return false;
    }

    st_reconstructor_control.key_collection = create_new_key_collection(key_reconstruction_collection_size);

    if (st_reconstructor_control.key_collection == NULL)
    {
        return false;
    }

    return true;
}


RECONSTRUCTION_QUEUEING_STATUS queue_key_for_reconstruction(uint16_t key_id, uint8_t key_fragment_no, uint8_t * encrypted_key_fragment, uint8_t * key_hmac, uint8_t xor_seed, const esp_bd_addr_t consumer_mac_address)
{
    RECONSTRUCTION_QUEUEING_STATUS result = QUEUED_SUCCESS;
    if (st_reconstructor_control.is_reconstructor_resources_init == true)
    {
        if (is_key_available(st_reconstructor_control.key_collection, consumer_mac_address, key_id) == true)
        {
            ESP_LOGI(REC_LOG_GROUP, "Key ID %d already in cache, skipping queue.", key_id);
            return QUEUED_FAILED_KEY_ALREADY_RECONSTRUCTED; // Key already reconstructed       
        }

        if (encrypted_key_fragment == NULL || key_hmac == NULL) {
            ESP_LOGE(REC_LOG_GROUP, "Invalid parameters in %s", __func__);
            result = QUEUED_FAILED_BAD_INPUT;
        }
        else
        {
            if (is_key_fragment_decrypted(st_reconstructor_control.key_collection, consumer_mac_address, key_id, key_fragment_no) == false)
            {
                reconstructor_queue_element q_in = {};
                q_in.key_id = key_id;
                q_in.key_fragment_no = key_fragment_no;
                q_in.xor_seed = xor_seed;

                memcpy(q_in.encrypted_key_fragment, encrypted_key_fragment, KEY_FRAGMENT_SIZE);
                memcpy(q_in.key_hmac, key_hmac, HMAC_SIZE);
                memcpy(q_in.consumer_mac_address, consumer_mac_address, sizeof(esp_bd_addr_t));

                const int MAX_RETRIES = 5;
                int queue_result;
                for (int i = 0; i < MAX_RETRIES; i++)
                {
                    queue_result = xQueueSend(st_reconstructor_control.xQueueKeyReconstruction, (void *)&q_in, QUEUE_TIMEOUT_MS / portTICK_PERIOD_MS);
                    if (queue_result == pdPASS)
                    {
                        xEventGroupSetBits(st_reconstructor_control.eventGroup, EVENT_NEW_KEY_FARGMENT_IN_QUEUE);
                        break;
                    }
                }
    
                if (queue_result != pdPASS) {
                    ESP_LOGE(REC_LOG_GROUP, "Queue send failed for key_id: %d", key_id);
                    result = QUEUED_FAILED_NO_SPACE;
                }
            }
            else
            {
                test_log_packet_received_key_fragment_already_decoded(consumer_mac_address);
                ESP_LOGE(REC_LOG_GROUP, "Key fragment %i is already decrypted", key_fragment_no);
            }
        }
    }
    else
    {
        ESP_LOGE(REC_LOG_GROUP, "Queue has not been initialized!");
        result = QUEEUD_FAILED_NOT_INITIALIZED;
    }

    return result;
}

void process_and_store_key_fragment(reconstructor_queue_element * q_element)
{
    ESP_LOGI(REC_LOG_GROUP, "Trying to reconstruct key fragment: %i", q_element->key_fragment_no);
    uint8_t decrypted_key_fragment_buffer[KEY_FRAGMENT_SIZE] = {0};
    uint8_t calculated_hmac_buffer[HMAC_SIZE] = {0};

    xor_decrypt_key_fragment(q_element->encrypted_key_fragment, decrypted_key_fragment_buffer, q_element->xor_seed);

    calculate_hmac_of_fragment(decrypted_key_fragment_buffer, q_element->encrypted_key_fragment, calculated_hmac_buffer);

    if (crypto_secure_memcmp(calculated_hmac_buffer, q_element->key_hmac, sizeof(calculated_hmac_buffer)) == 0)
    {
        add_fragment_to_key_management(st_reconstructor_control.key_collection, q_element->consumer_mac_address, q_element->key_id, decrypted_key_fragment_buffer, q_element->key_fragment_no);
        test_log_packet_received_key_fragment_already_decoded(q_element->consumer_mac_address);
        ESP_LOGI(REC_LOG_GROUP, "Successfully reconstructed key fragment no: %i", q_element->key_fragment_no);
    }
    else
    {
        test_log_bad_structure_packet(q_element->consumer_mac_address);
    }
}
