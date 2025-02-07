#include "test.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <string.h>

#include "esp_gap_ble_api.h"
#include "beacon_pdu_data.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define MAX_TEST_CONSUMERS 2
#define TEST_SEMAPHORE_MAX_BLOCK_TIME_MS 50
#define TEST_SEMAPHORE_MAX_BLOCK_TIME_SYSTICK pdMS_TO_TICKS(TEST_SEMAPHORE_MAX_BLOCK_TIME_MS)

#define PACKET_CONST_COUNTER 100
#define RECEIVER_TEST_TASK_STACK_SIZE 6000

uint8_t test_payload_4_bytes[4] = {0xb2, 0xaf, 0xc5, 0x6c};
uint8_t test_payload_10_bytes[10] = {0x74, 0x5d, 0xa3, 0x45, 0xa1, 0x1b, 0x0e, 0x02, 0x2a, 0x7f};
uint8_t test_payload_16_bytes[16] = {0x54, 0xeb, 0xca, 0x9d, 0x05, 0xff, 0x40, 0x49, 0x17, 0xa0, 0x3a, 0xd8, 0x77, 0x62, 0xed, 0xe2};

typedef struct {
    uint16_t key_id;
    uint64_t key_reconstruction_start_ms;
    uint64_t key_reconstruction_end_ms;
} key_reconstruction_info;

typedef struct {
    double total_fill;
    uint16_t no_checks;
} queue_fill_info;

typedef struct {
    esp_bd_addr_t mac_address;
    SemaphoreHandle_t xMutex;
    uint32_t total_packets_received;
    double total_key_reconstruction_time;
    double avarage_key_reconstruction_time;
    uint16_t no_reconstructed_keys;
    key_reconstruction_info key_rec_data;
    queue_fill_info deferred_queue;
    uint32_t no_bad_structure_packets;
    uint32_t wrongly_decoded_data_packets;
    uint32_t unauthorize_packets;
} test_consumer;

typedef struct {
    esp_bd_addr_t mac_address;
    uint32_t total_packets_send;
} test_producer;

typedef struct {
    uint64_t test_start_timestamp_ms;
    uint64_t test_end_timestamp_ms;
} test_duration;

static SemaphoreHandle_t xBleConsumerSemaphore = NULL;
static test_consumer ble_test_consumers[MAX_TEST_CONSUMERS] = {
    {.mac_address = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {.mac_address = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};
static queue_fill_info consumer_sec_processing_queue;
static test_producer ble_test_producer = {};
static test_duration test_duration_st = {0};
static esp_bd_addr_t zero_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static TEST_ROLE test_role;
static esp_bd_addr_t expected_sender_addrr[2] = {
    {0x1c,0x69, 0x20, 0x30, 0xde, 0x82},
    {0xd8, 0x3b, 0xda, 0xa0, 0xd1, 0x82}
};

static TaskHandle_t xTestPacketsTask;
static volatile QueueHandle_t xTestPacketsQueue;
static const uint16_t MAX_RECEIVER_TEST_QUEUE_SIZE = 70;
typedef struct {
    uint8_t packet[MAX_PDU_PAYLOAD_SIZE];
    size_t pdu_payload_size;
    esp_bd_addr_t mac_addr;
} test_packet_structure;
static const TickType_t TEST_QUEUE_WAIT_SYSTICKS = pdMS_TO_TICKS(50);

static uint32_t counter = 0;

void increment_received_packet_for_consumer(test_consumer * consumer)
{
    if (xSemaphoreTake(consumer->xMutex, TEST_SEMAPHORE_MAX_BLOCK_TIME_SYSTICK) == pdPASS)
    {
        consumer->total_packets_received++;
        xSemaphoreGive(consumer->xMutex);
    }
}

bool is_data_decoded_valid(uint8_t * data, size_t data_size)
{
    bool is_data_decoded_valid = false;
    switch (data_size)
    {

        case PAYLOAD_4_BYTES:
        {
            is_data_decoded_valid = memcmp(data, test_payload_4_bytes, data_size) == 0 ? true : false;
        }   
        break;

        case PAYLOAD_10_BYTES:
        {
            is_data_decoded_valid = memcmp(data, test_payload_10_bytes, data_size) == 0 ? true : false;
        }   
        break;

        case PAYLOAD_16_BYTES:
        {
            is_data_decoded_valid = memcmp(data, test_payload_16_bytes, data_size) == 0 ? true : false;
        }   
        break;

        default:
            break;

    };

    return is_data_decoded_valid;
}

bool is_pdu_from_expected_sender(esp_bd_addr_t addr)
{
    bool result = false;
    for (int i = 0; i < 2; i++)
    {
        result = memcmp(addr, expected_sender_addrr[i], sizeof(esp_bd_addr_t)) == 0 ? true : false;
        if (result == true)
        {
            break;
        }
    }
    return result;
}

bool is_queue_empty()
{
    return uxQueueMessagesWaiting(xTestPacketsQueue) == 0 ? true : false;
}

void reset_test_consumers_structure()
{
    test_duration_st.test_end_timestamp_ms = 0;
    test_duration_st.test_start_timestamp_ms = 0;

    for (int i = 0; i < MAX_TEST_CONSUMERS; i++)
    {
        ble_test_consumers[i].total_packets_received = 0;
        ble_test_consumers[i].total_key_reconstruction_time = 0;
        ble_test_consumers[i].avarage_key_reconstruction_time = 0;
        ble_test_consumers[i].no_reconstructed_keys = 0;
        ble_test_consumers[i].no_bad_structure_packets = 0;
        ble_test_consumers[i].wrongly_decoded_data_packets = 0;
        ble_test_consumers[i].key_rec_data.key_id = 0;
        ble_test_consumers[i].key_rec_data.key_reconstruction_start_ms = 0;
        ble_test_consumers[i].key_rec_data.key_reconstruction_end_ms = 0;
        ble_test_consumers[i].deferred_queue.no_checks = 0;
        ble_test_consumers[i].deferred_queue.total_fill = 0;
        ble_test_consumers[i].unauthorize_packets = 0;
        memset(ble_test_consumers[i].mac_address, 0, sizeof(esp_bd_addr_t));

        ble_test_consumers[i].xMutex = xSemaphoreCreateMutex();
        if (ble_test_consumers[i].xMutex == NULL)
        {
            ESP_LOGE(TEST_ESP_LOG_GROUP, "Failed to initialized Semaphore");
        }
    }

    memset(ble_test_producer.mac_address, 0, sizeof(esp_bd_addr_t));
    ble_test_producer.total_packets_send = 0;

    consumer_sec_processing_queue.no_checks = 0;
    consumer_sec_processing_queue.total_fill = 0;

}

int get_consumer_index(esp_bd_addr_t mac_address)
{
    int index = -1;
    if (xSemaphoreTake(xBleConsumerSemaphore, TEST_SEMAPHORE_MAX_BLOCK_TIME_SYSTICK) == pdTRUE)
    {
        for (int i = 0; i < MAX_TEST_CONSUMERS; i++)
        {
            if (memcmp(ble_test_consumers[i].mac_address, mac_address, sizeof(esp_bd_addr_t)) == 0)
            {
                index = i;
                break;
            }
        }
        xSemaphoreGive(xBleConsumerSemaphore);
    }
    return index;
}

int add_consumer_to_table(esp_bd_addr_t mac_address)
{
    int index = -1;
    if (xSemaphoreTake(xBleConsumerSemaphore, TEST_SEMAPHORE_MAX_BLOCK_TIME_SYSTICK) == pdTRUE)
    {
        for (int i = 0; i < MAX_TEST_CONSUMERS; i++)
        {
            if (memcmp(ble_test_consumers[i].mac_address, mac_address, sizeof(esp_bd_addr_t)) == 0)
            {
                index = i;
                break;
            }
            else if (memcmp(ble_test_consumers[i].mac_address, zero_mac, sizeof(esp_bd_addr_t)) == 0)
            {
                memcpy(ble_test_consumers[i].mac_address, mac_address, sizeof(esp_bd_addr_t));
                index = i;
                break;
            }
        }
        xSemaphoreGive(xBleConsumerSemaphore);
    }
    return index;
}

void init_test()
{
    reset_test_consumers_structure();
    if (xBleConsumerSemaphore == NULL)
    {
        xBleConsumerSemaphore = xSemaphoreCreateMutex();
        if (xBleConsumerSemaphore == NULL)
        {
            ESP_LOGI(TEST_ESP_LOG_GROUP, "Failed to initialized Semaphore");
            return;
        }
    }
}

void handle_queue_for_sender(test_packet_structure * pdu);
void handle_queue_for_receiver(test_packet_structure * pdu);

void test_receiver_app_main(void *arg)
{
    test_packet_structure queue_element = {};
    const uint32_t DELAY_TO_NEXT_LOG_SYSTICK = pdMS_TO_TICKS(20);
    TEST_ROLE role = test_role; 
    while (1)
    {
        if (xQueueReceive(xTestPacketsQueue, &queue_element, TEST_QUEUE_WAIT_SYSTICKS) == pdTRUE)
        {
            switch (role)
            {
                case TEST_RECEIVER_ROLE:
                {
                    handle_queue_for_receiver(&queue_element);
                }
                break;

                case TEST_SENDER_ROLE:
                {
                    handle_queue_for_sender(&queue_element);
                }
                break;
            }

            vTaskDelay(DELAY_TO_NEXT_LOG_SYSTICK);
        }
    }
}

void handle_queue_for_sender(test_packet_structure * pdu)
{
    ESP_LOGI(TEST_ESP_LOG_GROUP, "Packet %lu has been sent!", ++ble_test_producer.total_packets_send);
    ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: Packet payload", pdu->packet, pdu->pdu_payload_size, 0);
}

void handle_queue_for_receiver(test_packet_structure * pdu)
{
    int index = -1;

    if ((index = get_consumer_index(pdu->mac_addr)) >= 0)
    {
        ESP_LOGI(TEST_ESP_LOG_GROUP, "Packet %lu has been received!", ++ble_test_consumers[index].total_packets_received);
        ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: Packet received from", pdu->mac_addr, sizeof(esp_bd_addr_t), 0);
        ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: Packet payload", pdu->packet, pdu->pdu_payload_size, 0);
        if (is_data_decoded_valid(pdu->packet, pdu->pdu_payload_size) == false)
        {
            ESP_LOGE(TEST_ESP_LOG_GROUP, "Packet payload is wrong after decoding!");
            ble_test_consumers[index].wrongly_decoded_data_packets++;
        }
    }
    else
    {
        if ((index = add_consumer_to_table(pdu->mac_addr)) >= 0)
        {
            ESP_LOGI(TEST_ESP_LOG_GROUP, "Packet %lu has been received!", ++ble_test_consumers[index].total_packets_received);
            ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: Packet received from", pdu->mac_addr, sizeof(esp_bd_addr_t), 0);
            ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: Packet payload", pdu->packet, pdu->pdu_payload_size, 0);
            if (is_data_decoded_valid(pdu->packet, pdu->pdu_payload_size) == false)
            {
                ESP_LOGE(TEST_ESP_LOG_GROUP, "Packet payload is wrong after decoding!");
                ble_test_consumers[index].wrongly_decoded_data_packets++;
            }
        }
    }
}


void init_task_resources()
{
    xTestPacketsQueue = xQueueCreate(MAX_RECEIVER_TEST_QUEUE_SIZE, sizeof(test_packet_structure));
    if (xTestPacketsQueue == NULL)
    {
        ESP_LOGE(TEST_ESP_LOG_GROUP, "Failed to initialized test receiver queue");
        return;
    }

    int result = xTaskCreate(
        test_receiver_app_main,
        "RECEIVER TEST TASK",
        RECEIVER_TEST_TASK_STACK_SIZE,
        NULL,
        14,
        &xTestPacketsTask
    );

    if (xTestPacketsTask == NULL || result != pdPASS)
    {
        ESP_LOGE(TEST_ESP_LOG_GROUP, "Failed to initialized test receiver task");
        return;
    }
}

void start_test_measurment(TEST_ROLE role)
{
    test_role = role;
    //init_task_resources();
    ESP_LOGI(TEST_ESP_LOG_GROUP, "--------------TEST STARTED------------");
    ESP_LOGI(TEST_ESP_LOG_GROUP, "--------------%s------------", role == TEST_SENDER_ROLE ? "SENDER" : "RECEIVER");
    test_duration_st.test_start_timestamp_ms = esp_timer_get_time() / 1000;
}

void end_test_measurment()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // while (is_queue_empty() == false)
    // {
    //     vTaskDelay(pdMS_TO_TICKS(100));
    // }

    test_duration_st.test_end_timestamp_ms = esp_timer_get_time() / 1000;
    int test_duration_s = (int)((test_duration_st.test_end_timestamp_ms - test_duration_st.test_start_timestamp_ms) / 1000);

    ESP_LOGI(TEST_ESP_LOG_GROUP, "TEST DURATION TIME IN S: %i", test_duration_s);
    if (test_role == TEST_RECEIVER_ROLE)
    {
        double avarage_sec_processing_fill = (consumer_sec_processing_queue.total_fill / consumer_sec_processing_queue.no_checks) * 100;
        ESP_LOGI(TEST_ESP_LOG_GROUP, "SEC PROCESSING QUEU AVARAGE FILL: %f", avarage_sec_processing_fill);
        for (int i = 0; i < MAX_TEST_CONSUMERS; i++)
        {
            if (memcmp(ble_test_consumers[i].mac_address, zero_mac, sizeof(esp_bd_addr_t)) != 0)
            {
                ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: CONSUMER ADDR", ble_test_consumers[i].mac_address, sizeof(esp_bd_addr_t), 0);
                ESP_LOGI(TEST_ESP_LOG_GROUP, "TOTAL PACKET RECEIVED: %lu", ble_test_consumers[i].total_packets_received);
                ESP_LOGI(TEST_ESP_LOG_GROUP, "AVARAGE KEY RECONSTRUCTION TIME IN S: %.2f", (double) (ble_test_consumers[i].avarage_key_reconstruction_time / 1000));
                ESP_LOGI(TEST_ESP_LOG_GROUP, "NO KEY RECONSTRUCTED: %i", (int) ble_test_consumers[i].no_reconstructed_keys);
                ESP_LOGI(TEST_ESP_LOG_GROUP, "NO BAD STRUCTURE PACKETS: %i", (int) ble_test_consumers[i].no_bad_structure_packets);
                ESP_LOGI(TEST_ESP_LOG_GROUP, "WRONGLY DECODED PACKETS: %i", (int) ble_test_consumers[i].wrongly_decoded_data_packets);
                ESP_LOGI(TEST_ESP_LOG_GROUP, "UNAUTHORIZE INTERVAL PACKETS: %i", (int) ble_test_consumers[i].unauthorize_packets);
                if (ble_test_consumers[i].deferred_queue.no_checks != 0)
                {
                    double avarage_def_q_fill = ((double)((ble_test_consumers[i].deferred_queue.total_fill * 100) / ((double) ble_test_consumers[i].deferred_queue.no_checks)) );
                    ESP_LOGI(TEST_ESP_LOG_GROUP, "DEFERRED QUEUE AVARAGE FILL WHILE KEY RECONSTRUCTION COMPLETE: %.2f", avarage_def_q_fill);
                }
            }
        }
    }
    else
    {
        ESP_LOGI(TEST_ESP_LOG_GROUP, "TOTAL PACKET SEND: %lu", ble_test_producer.total_packets_send);
    }
    ESP_LOGI(TEST_ESP_LOG_GROUP, "--------------TEST ENDED------------");
}

void test_log_sender_data(size_t payload_size, int adv_interval)
{
    ESP_LOGI(TEST_ESP_LOG_GROUP, "----------ADVERTISEMENT SETTINGS-------------");
    ESP_LOGI(TEST_ESP_LOG_GROUP, "ADVERTISEMENT INTERVAL: %i", adv_interval);
    ESP_LOGI(TEST_ESP_LOG_GROUP, "ADVERTISEMENT PAYLOAD SIZE: %i", (int) payload_size);
}

void test_log_sender_key_replace_time_in_s(uint16_t key_replace_time_in_s)
{
    ESP_LOGI(TEST_ESP_LOG_GROUP, "----------KEY REPLACE TIME-------------");
    ESP_LOGI(TEST_ESP_LOG_GROUP, "KEY REPLACE TIME: %i", (int) key_replace_time_in_s);
}

void test_log_packet_received(uint8_t *data, size_t data_len, esp_bd_addr_t mac_address)
{

    if (data == NULL || mac_address == NULL)
    {
        ESP_LOGE(TEST_ESP_LOG_GROUP, "Passed PDU Data or Mac Addrr is NULL");
        return;
    }

    int index = -1;
    if ((index = get_consumer_index(mac_address)) >= 0)
    {
        if (is_data_decoded_valid(data, data_len) == false)
        {
            ble_test_consumers[index].wrongly_decoded_data_packets++;
        }
        else
        {
            increment_received_packet_for_consumer(&ble_test_consumers[index]);

            if (ble_test_consumers[index].total_packets_received % PACKET_CONST_COUNTER == 0)
            {
                counter++;
                ESP_LOG_BUFFER_HEX("TEST_LOG_GROUP: Sender addr", data, data_len);
                ESP_LOGI(TEST_ESP_LOG_GROUP, "%lu Packet has been received!", (uint32_t) (counter * PACKET_CONST_COUNTER));
            }

            
        }
    }
    else
    {
        if ((index = add_consumer_to_table(mac_address)) >= 0)
        {
            if (is_data_decoded_valid(data, data_len) == false)
            {
                ble_test_consumers[index].wrongly_decoded_data_packets++;
            }
            else
            {
                increment_received_packet_for_consumer(&ble_test_consumers[index]);
                if (ble_test_consumers[index].total_packets_received % PACKET_CONST_COUNTER == 0)
                {
                    counter++;
                    ESP_LOG_BUFFER_HEX("TEST_LOG_GROUP: Sender addr", data, data_len);
                    ESP_LOGI(TEST_ESP_LOG_GROUP, "%lu Packet has been received!", (uint32_t) (counter * PACKET_CONST_COUNTER));
                }
            }
        }
    }
}

void test_log_packet_received_key_fragment_already_decoded(esp_bd_addr_t mac_address)
{
    int index = -1;
    if ((index = get_consumer_index(mac_address)) >= 0)
    {
        increment_received_packet_for_consumer(&ble_test_consumers[index]);
    }
}

void test_log_bad_structure_packet(esp_bd_addr_t addr)
{
    if (addr == NULL)
        return;
        
    int index = -1;
    if ((index = get_consumer_index(addr)) >= 0)
    {
        ble_test_consumers[index].no_bad_structure_packets++;
    }
    else
    {
        if ((index = add_consumer_to_table(addr)) >= 0)
        {
            ble_test_consumers[index].no_bad_structure_packets++;
        }
    }
}

void test_log_packet_send(uint8_t *data, size_t data_len, esp_bd_addr_t mac_address)
{
    if (data == NULL)
    {
        ESP_LOGE(TEST_ESP_LOG_GROUP, "Passed PDU Data or Mac Addrr is NULL");
        return;
    }
    ++ble_test_producer.total_packets_send;
    if (ble_test_producer.total_packets_send % PACKET_CONST_COUNTER == 0)
    {
        counter++;
        ESP_LOGI(TEST_ESP_LOG_GROUP, "%lu Packet has been sent!", (uint32_t) (counter * PACKET_CONST_COUNTER));
    }
}

void test_log_key_fragment_send()
{
    ++ble_test_producer.total_packets_send;
    if (ble_test_producer.total_packets_send % PACKET_CONST_COUNTER == 0)
    {
        counter++;
        ESP_LOGI(TEST_ESP_LOG_GROUP, "%lu Packet has been sent!", (uint32_t) (counter * PACKET_CONST_COUNTER));
    }
}

void test_log_key_reconstruction_start(esp_bd_addr_t mac_address, uint16_t key_id)
{
    int index = -1;
    if ((index = get_consumer_index(mac_address)) >= 0)
    {
        ble_test_consumers[index].key_rec_data.key_reconstruction_start_ms = esp_timer_get_time() / 1000;
    }
    {
        if ((index = add_consumer_to_table(mac_address)) >= 0)
        {
            ble_test_consumers[index].key_rec_data.key_reconstruction_start_ms = esp_timer_get_time() / 1000;
        }
    }
}

void test_log_key_reconstruction_end(esp_bd_addr_t mac_address, uint16_t key_id)
{
    static uint16_t prev_logged_key_id = 0;
    if (prev_logged_key_id != key_id)
    {
        int index = -1;
        if ((index = get_consumer_index(mac_address)) >= 0)
        {
            ble_test_consumers[index].key_rec_data.key_reconstruction_end_ms = esp_timer_get_time() / 1000;
            ble_test_consumers[index].no_reconstructed_keys++;
            ble_test_consumers[index].total_key_reconstruction_time += ble_test_consumers[index].key_rec_data.key_reconstruction_end_ms - ble_test_consumers[index].key_rec_data.key_reconstruction_start_ms;
            ble_test_consumers[index].avarage_key_reconstruction_time = (double) (ble_test_consumers[index].total_key_reconstruction_time / ble_test_consumers[index].no_reconstructed_keys);
        }
        ESP_LOGI(TEST_ESP_LOG_GROUP, "KeyID %i has been reconstructed in time ms %i", (int) key_id, (int)(ble_test_consumers[index].key_rec_data.key_reconstruction_end_ms - ble_test_consumers[index].key_rec_data.key_reconstruction_start_ms));
    }
}

void test_log_deferred_queue_percentage(double percentage, esp_bd_addr_t mac_address)
{
    int index = -1;
    if ((index = get_consumer_index(mac_address)) >= 0)
    {
        ble_test_consumers[index].deferred_queue.no_checks++;
        ble_test_consumers[index].deferred_queue.total_fill += percentage;
        ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: CONSUMER ADDR", ble_test_consumers[index].mac_address, sizeof(esp_bd_addr_t), 0);
        ESP_LOGI(TEST_ESP_LOG_GROUP, "Deferred Queue Fill: %.2f", percentage * 100.0);
    }
    {
        if ((index = add_consumer_to_table(mac_address)) >= 0)
        {
            ble_test_consumers[index].deferred_queue.no_checks++;
            ble_test_consumers[index].deferred_queue.total_fill += percentage;
            ESP_LOG_BUFFER_HEXDUMP("TEST_LOG_GROUP: CONSUMER ADDR", ble_test_consumers[index].mac_address, sizeof(esp_bd_addr_t), 0);
            ESP_LOGI(TEST_ESP_LOG_GROUP, "Deferred Queue Fill: %.2f", percentage * 100.0);
        }
    }
}

void test_log_processing_queue_percentage(double percentage)
{
    ESP_LOGI(TEST_ESP_LOG_GROUP, "Sec Processing Queue Fill: %.2f", percentage * 100.0);
    consumer_sec_processing_queue.no_checks++;
    consumer_sec_processing_queue.total_fill += percentage;
}

void test_log_adv_time_not_authorize(esp_bd_addr_t addr)
{
    int index = -1;
    if ((index = get_consumer_index(addr)) >= 0)
    {
        ble_test_consumers[index].unauthorize_packets++;
    }
    {
        if ((index = add_consumer_to_table(addr)) >= 0)
        {
            ble_test_consumers[index].unauthorize_packets++;
        }
    }
}
