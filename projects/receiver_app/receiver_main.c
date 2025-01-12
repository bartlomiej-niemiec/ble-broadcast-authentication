 
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "beacon_pdu_data.h"
#include "beacon_test_pdu.h"
#include "ble_gap_payload_consuming.h"
#include "sec_pdu_processing.h"
#include "ble_broadcast_controller.h"
#include "test.h"


#include "freertos/FreeRTOS.h"

#define EVENT_START_PDU (1 << 0)
#define EVENT_END_PDU (1 << 1)

#define EVENT_GROUP_WAIT_MS 50
#define EVENT_GROUP_WAIT_SYSTICK pdMS_TO_TICKS(EVENT_GROUP_WAIT_MS)

static const char * BLE_GAP_LOG_GROUP = "BLE_RECEIVER";

typedef enum {
    RECEIVER_WAIT_FOR_TEST_START_PDU,
    RECEIVER_SCANNING_PDUS,
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

void ble_receiver_main_loop();
void handle_wait_for_start_pdu(int *state, EventBits_t events);
void handle_scanning_pdus(int *state, EventBits_t events);
void receiver_app_scan_complete_callback(int64_t timestamp_us, uint8_t *data, size_t data_size, esp_bd_addr_t mac_address);

void app_main(void)
{
    receiverAppEventGroup = xEventGroupCreate();
    if (receiverAppEventGroup == NULL)
        return;

    int sec_pdu_status = start_up_sec_processing();
    if (sec_pdu_status == 0)
    {
        register_payload_observer_cb(payload_notifier);
        ESP_LOGI(BLE_GAP_LOG_GROUP, "Sec PDU Creation Success");
    }
    else
    {
        ESP_LOGI(BLE_GAP_LOG_GROUP, "Sec PDU Creation Failed: %i", sec_pdu_status);
        return;
    }

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

            default:
                break;
        };
    }
}

void handle_wait_for_start_pdu(int *state, EventBits_t events)
{
    if (events & EVENT_START_PDU)
    {
        init_test();
        start_test_measurment();
        *state = RECEIVER_SCANNING_PDUS;
    }
}

void handle_scanning_pdus(int *state, EventBits_t events)
{
    if (events & EVENT_END_PDU)
    {
        end_test_measurment();
        *state = RECEIVER_WAIT_FOR_TEST_START_PDU;
    }
}

void receiver_app_scan_complete_callback(int64_t timestamp_us, uint8_t *data, size_t data_size, esp_bd_addr_t mac_address)
{
    if (data == NULL)
        return;

    if (is_test_pdu(data, data_size) == ESP_OK)
    {
        if (is_test_start_pdu(data, data_size) == ESP_OK)
        {
            ESP_LOGI(BLE_GAP_LOG_GROUP, "Received test start PDU");
            xEventGroupSetBits(receiverAppEventGroup, EVENT_START_PDU);
        }
        else if (is_test_end_pdu(data, data_size) == ESP_OK)
        {
            ESP_LOGI(BLE_GAP_LOG_GROUP, "Received test end PDU");
            xEventGroupSetBits(receiverAppEventGroup, EVENT_END_PDU);
        }
    }
}

