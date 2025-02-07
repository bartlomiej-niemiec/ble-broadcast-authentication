 
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "beacon_pdu_data.h"
#include "beacon_test_pdu.h"
#include "sec_pdu_processing.h"
#include "sec_pdu_processing_scan_callback.h"
#include "ble_broadcast_controller.h"
#include "test.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define EVENT_START_PDU (1 << 0)
#define EVENT_END_PDU (1 << 1)

#define EVENT_GROUP_WAIT_MS 50
#define EVENT_GROUP_WAIT_SYSTICK pdMS_TO_TICKS(EVENT_GROUP_WAIT_MS)

#define SENDERS_NUMBER 1

static const char * BLE_GAP_LOG_GROUP = "BLE_RECEIVER";

typedef enum {
    RECEIVER_WAIT_FOR_TEST_START_PDU,
    RECEIVER_SCANNING_PDUS,
    RECEIVER_CLEANUP_STATE
} receiver_state_machine;

static esp_ble_scan_params_t default_ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 160,
    .scan_window            = 160,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

static EventGroupHandle_t receiverAppEventGroup;

typedef struct 
{
    esp_bd_addr_t address;
    bool test_end_received;
} active_test_senders;

static const esp_bd_addr_t zero_address_init = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static active_test_senders test_senders[2] = {
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        false
    },
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        false
    }
};

bool received_all_test_ends()
{
    bool result = false;
    int count = 0;
    for (int i = 0; i < SENDERS_NUMBER; i++)
    {
        if (test_senders[i].test_end_received == true)
        {
            count++;
        }
    }

    return count == SENDERS_NUMBER ? true : false;
}

void check_and_add_test_sender(esp_bd_addr_t mac_address)
{
    for (int i = 0; i < SENDERS_NUMBER; i++)
    {
        int result = memcmp(test_senders[i].address, mac_address, sizeof(esp_bd_addr_t));
        int is_zero = memcmp(test_senders[i].address, zero_address_init, sizeof(esp_bd_addr_t));
        if (result != 0 && is_zero == 0)
        {
            memcpy(test_senders[i].address, mac_address, sizeof(esp_bd_addr_t));
            test_senders[i].test_end_received = true;
            break;
        }
        else if (is_zero != 0 && result == 0)
        {
            break;
        }
    }
}


static SemaphoreHandle_t sem_active_senders = NULL;
static volatile uint16_t active_test_senders_counter = 0;

void ble_receiver_main_loop();
void handle_wait_for_start_pdu(int *state, EventBits_t events);
void handle_scanning_pdus(int *state, EventBits_t events);
void handle_cleanup_state(int *state);
void receiver_app_scan_complete_callback(int64_t timestamp_us, uint8_t *data, size_t data_size, esp_bd_addr_t mac_address);

void app_main(void)
{
    receiverAppEventGroup = xEventGroupCreate();
    sem_active_senders = xSemaphoreCreateMutex();
    if (receiverAppEventGroup == NULL)
        return;

    if (sem_active_senders == NULL)
        return;

    int sec_pdu_status = start_up_sec_processing();
    if (sec_pdu_status == 0)
    {
        register_payload_observer_cb(test_log_packet_received);
        ESP_LOGI(BLE_GAP_LOG_GROUP, "Sec PDU Creation Success");
    }
    else
    {
        ESP_LOGI(BLE_GAP_LOG_GROUP, "Sec PDU Creation Failed: %i", sec_pdu_status);
        return;
    }

    init_test();

    bool init_stat = init_broadcast_controller();
    if (init_stat == true)
    {
        register_scan_complete_callback(scan_complete_callback);
        register_scan_complete_callback(receiver_app_scan_complete_callback);
        start_scanning(default_ble_scan_params, 0);
    }
    else
    {
        ESP_LOGE(BLE_GAP_LOG_GROUP, "Failed initializtion of broadcast controller");
    }

    ble_receiver_main_loop();
}


void ble_receiver_main_loop()
{
    receiver_state_machine state = RECEIVER_WAIT_FOR_TEST_START_PDU;

    while (true)
    {
        EventBits_t events = xEventGroupWaitBits(receiverAppEventGroup,
                                                 EVENT_START_PDU | EVENT_END_PDU,
                                                 pdTRUE, pdFALSE, EVENT_GROUP_WAIT_SYSTICK);

        switch (state)
        {
            case RECEIVER_WAIT_FOR_TEST_START_PDU:
            {
                handle_wait_for_start_pdu((int *) &state, events);
            }
            break;


            case RECEIVER_SCANNING_PDUS:
            {
                handle_scanning_pdus((int *) &state, events);
            }
            break;

            case RECEIVER_CLEANUP_STATE:
            {
                handle_cleanup_state((int *) &state);
            }
            break;

            default:
                break;
        };
    }
}

void handle_wait_for_start_pdu(int *state, EventBits_t events)
{
    if (events & EVENT_START_PDU)
    {
        start_test_measurment(TEST_RECEIVER_ROLE);
        *state = RECEIVER_SCANNING_PDUS;
    }
}

void handle_scanning_pdus(int *state, EventBits_t events)
{
    if (events & EVENT_END_PDU)
    {
        *state = RECEIVER_CLEANUP_STATE;
    }
}

void handle_cleanup_state(int *state)
{
    static const uint32_t timeDelayMs = 10 * 1e3;
    vTaskDelay(pdMS_TO_TICKS(timeDelayMs));
    end_test_measurment();
    *state = RECEIVER_WAIT_FOR_TEST_START_PDU;
}

void receiver_app_scan_complete_callback(int64_t timestamp_us, uint8_t *data, size_t data_size, esp_bd_addr_t mac_address)
{
    if (data == NULL)
        return;

    static bool received_test_start = false;
    static bool received_test_end = false;

    if (is_test_pdu(data, data_size) == ESP_OK)
    {
        if (is_test_start_pdu(data, data_size) == ESP_OK)
        {
            if (received_test_start == false)
            {
                ESP_LOGI(BLE_GAP_LOG_GROUP, "Received test start PDU");
                xEventGroupSetBits(receiverAppEventGroup, EVENT_START_PDU);
                received_test_start = true;
            }
        }
        else if (is_test_end_pdu(data, data_size) == ESP_OK)
        {
            check_and_add_test_sender(mac_address);
            if (received_all_test_ends() == true)
            {
                ESP_LOGI(BLE_GAP_LOG_GROUP, "Received test end PDU");
                xEventGroupSetBits(receiverAppEventGroup, EVENT_END_PDU);
                received_test_end = true;
            }
        }
    }
}

