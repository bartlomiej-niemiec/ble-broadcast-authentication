#include "ble_security_payload_encryption.h"
#include "ble_broadcast_controller.h"
#include "esp_log.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_random.h"

#include "pc_serial_communication.h"
#include "beacon_test_pdu.h"

#include "test.h"

#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>

#define MAX_SERIAL_MSG_SIZE 30

#define TEST_DURATION_IN_S 180

typedef enum {
    SENDER_WAIT_FOR_START_CMD,
    SENDER_TEST_START_PDU,
    SENDER_BROADCAST_PDU,
    SENDER_TEST_END_PDU
} sender_state_machine;

volatile uint32_t packet_send_counter = 0;
volatile bool is_first_pdu = true;
static const char* SENDER_APP_LOG_GROUP = "SENDER APP";
static char PC_SERIAL_BUFFER[MAX_SERIAL_MSG_SIZE] = {0};
static const char* START_CMD_SERIAL = "START_TEST";
static SemaphoreHandle_t xStartCmdReceived;
static atomic_int EndTest = 0;
static esp_timer_handle_t xTestTimeoutTimer;
static uint64_t testTimeoutUs = TEST_DURATION_IN_S * 1e6;
static uint8_t *test_payload_buffer_ptr = NULL;

#define ADV_INT_PLUS_10(x) (int)((x) + (((double)(x)) * (0.1)))

#define TEST_PAYLOAD_BYTES_LEN PAYLOAD_10_BYTES
#define TEST_ADV_INTERVAL INT_RANDOM

#define ADV_INT_MIN_MS TEST_ADV_INTERVAL
#define ADV_INT_MAX_MS TEST_ADV_INTERVAL //ADV_INT_PLUS_10(TEST_ADV_INTERVAL)

#define TASK_DELAY_MS ADV_INT_MIN_MS
#define TASK_DELAY_SYSTICK pdMS_TO_TICKS(TASK_DELAY_MS)

#define N_CONST 0.625
#define MS_TO_N_CONVERTION(MS) ((uint16_t)((MS) / (N_CONST)))

#define MAX_BLOCK_TIME_SEMAPHORE_MS 300
#define MAX_BLOCK_TIME_SEMAPHORE_TICKS pdMS_TO_TICKS(MAX_BLOCK_TIME_SEMAPHORE_MS)

#define NO_PACKET_TO_SEND 2000
#define TEST_NO_PACKETS_TO_KEY_REPLACE 200

#define PDU_TO_KEY_FRAGMENT_RATIO 6

static volatile uint16_t pdu_send_counter = PDU_TO_KEY_FRAGMENT_RATIO;
static volatile uint32_t prev_key_id = 0;

uint16_t get_and_increment_pdu_send_counter()
{
    uint16_t counter_value = pdu_send_counter;
    if (pdu_send_counter == PDU_TO_KEY_FRAGMENT_RATIO)
    {
        pdu_send_counter = 0;   
    }
    else
    {
        pdu_send_counter++;
    }
    return counter_value;
}


static esp_ble_adv_params_t default_ble_adv_params = {
    .adv_int_min        = MS_TO_N_CONVERTION(ADV_INT_MIN_MS),
    .adv_int_max        = MS_TO_N_CONVERTION(ADV_INT_MAX_MS),
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

static volatile uint32_t current_random_interval_ms = ADV_INT_MIN_MS;

void test_timeout_callback(void *arg)
{
   atomic_store(&EndTest, 1);
}
static esp_timer_create_args_t testTimeoutTimer = {
  .callback = test_timeout_callback,
  .arg = NULL, 
  .name = "TEST_TIMEOUT_TIMER",  
};

static volatile uint32_t no_send_pdus = 0;

void ble_sender_main();
bool encrypt_new_payload();
bool encrypt_new_key_fragment();
void data_set_success_cb()
{
    no_send_pdus++;
}

void serial_data_received(uint8_t * data, size_t data_len)
{
    memcpy(PC_SERIAL_BUFFER, (char *) data, data_len);
    PC_SERIAL_BUFFER[data_len] = '\0';
    ESP_LOGI(SENDER_APP_LOG_GROUP, "PC SERIAL MSG: %s", PC_SERIAL_BUFFER);
    if (strstr(PC_SERIAL_BUFFER, START_CMD_SERIAL) != NULL)
    {
        xSemaphoreGive(xStartCmdReceived);
    }
}

uint8_t * get_test_payload_buffer(size_t data_len)
{
    uint8_t * payload_buffer_ptr = NULL;
    switch (data_len)
    {
        case PAYLOAD_4_BYTES:
        {
            payload_buffer_ptr = test_payload_4_bytes;
        }
        break;

        case PAYLOAD_10_BYTES:
        {
            payload_buffer_ptr = test_payload_10_bytes;
        }
        break;

        case PAYLOAD_16_BYTES:
        {
            payload_buffer_ptr = test_payload_16_bytes;
        }   
        break;

        default:
            break;
    };


    return payload_buffer_ptr;
}

void app_main(void)
{
    init_payload_encryption();
    bool init_controller = init_broadcast_controller();

    //ASSIGN PAYLOAD BUFFER
    if (TEST_PAYLOAD_BYTES_LEN != RANDOM_SIZE)
    {
        test_payload_buffer_ptr = get_test_payload_buffer(TEST_PAYLOAD_BYTES_LEN);
        if (test_payload_buffer_ptr == NULL)
        {
            ESP_LOGE(SENDER_APP_LOG_GROUP, "Failed acquire test payload buffer!");
            return;
        }
    }

    xStartCmdReceived = xSemaphoreCreateBinary();
    if (xStartCmdReceived == NULL)
        return;
    xSemaphoreTake(xStartCmdReceived, MAX_BLOCK_TIME_SEMAPHORE_TICKS);

    if (esp_timer_create(&testTimeoutTimer, &xTestTimeoutTimer) != ESP_OK)
        return;

    if (init_controller == true)
    {
        register_broadcast_new_data_callback(data_set_success_cb);

        if (start_pc_serial_communication() != ESP_OK)
        {
            ESP_LOGE(SENDER_APP_LOG_GROUP, "Failed initializtion of pc communication");
            return;
        }
        register_serial_data_received_cb(serial_data_received);

        ble_sender_main();
    }
    else
    {
        ESP_LOGE(SENDER_APP_LOG_GROUP, "Failed to init broadcast controller");
    }
}

void sender_wair_for_start_cmd(int * state);
void sender_test_start_pdu(int * state);
void sender_broadcast_pdu(int * state);
void sender_test_end_pdu(int * state);
bool encrypt_new_payload();

void ble_sender_main()
{
    int state_machine_state = SENDER_WAIT_FOR_START_CMD;
    while (1)
    {
        switch (state_machine_state)
        {
            case SENDER_WAIT_FOR_START_CMD:
            {
                sender_wair_for_start_cmd(&state_machine_state);
            }
            break;

            case SENDER_TEST_START_PDU:
            {
                sender_test_start_pdu(&state_machine_state);
            }
            break;

            case SENDER_BROADCAST_PDU:
            {
                sender_broadcast_pdu(&state_machine_state);
            }
            break;

            case SENDER_TEST_END_PDU:
            {
                sender_test_end_pdu(&state_machine_state);
            }
            break;
        }
    }
}

void sender_wair_for_start_cmd(int *state)
{
    if (xSemaphoreTake(xStartCmdReceived, MAX_BLOCK_TIME_SEMAPHORE_TICKS) == pdPASS)
    {
        ESP_LOGI(SENDER_APP_LOG_GROUP, "Received Start Command From Pc");
        *state = SENDER_TEST_START_PDU;
    }
    vTaskDelay(TASK_DELAY_SYSTICK);
}

void sender_test_start_pdu(int *state)
{
    ESP_LOGI(SENDER_APP_LOG_GROUP, "Sending Start Broadcast PDUs");
    beacon_test_pdu pdu;
    if (build_test_start_pdu(&pdu) == ESP_OK)
    {
        set_broadcasting_payload((uint8_t *)&pdu, sizeof(beacon_test_pdu));
    }
    
    start_broadcasting(&default_ble_adv_params);
    if (TEST_ADV_INTERVAL == INT_20MS)
    {
        vTaskDelay(pdMS_TO_TICKS(TEST_ADV_INTERVAL * 30));
    }
    else if (TEST_ADV_INTERVAL == INT_RANDOM)
    {
        ESP_LOGI(SENDER_APP_LOG_GROUP, "Wait a bit");
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
    else
    {
        vTaskDelay(pdMS_TO_TICKS(TEST_ADV_INTERVAL * 10));
    }
    init_test();
    start_test_measurment(TEST_SENDER_ROLE);
    test_log_sender_data(TEST_PAYLOAD_BYTES_LEN, TEST_ADV_INTERVAL);
    ESP_LOGI(SENDER_APP_LOG_GROUP, "Changing state to broadcast pdus");
    //esp_timer_start_once(xTestTimeoutTimer, testTimeoutUs);
    *state = SENDER_BROADCAST_PDU;
}

void sender_broadcast_pdu(int *state)
{
    if (packet_send_counter >= NO_PACKET_TO_SEND)
    {
        *state = SENDER_TEST_END_PDU;
        return;
    }

    bool result;
    if (get_and_increment_pdu_send_counter() == PDU_TO_KEY_FRAGMENT_RATIO)
    {
        result = encrypt_new_key_fragment();
    }
    else
    {
        result = encrypt_new_payload();
    }
    if (!result)
    {
        vTaskDelay(pdMS_TO_TICKS(current_random_interval_ms)); // Wait and continue the loop
        return;
    }
        
    vTaskDelay(pdMS_TO_TICKS(current_random_interval_ms)); // Wait before sending the next payload
}

void sender_test_end_pdu(int *state)
{
    beacon_test_pdu pdu;
    if (build_test_end_pdu(&pdu) == ESP_OK)
    {
        set_broadcasting_payload((uint8_t *)&pdu, sizeof(beacon_test_pdu));
    }
    if (TEST_ADV_INTERVAL == INT_20MS)
    {
        vTaskDelay(TEST_ADV_INTERVAL * 30);
    }
    else
    {
        vTaskDelay(TEST_ADV_INTERVAL * 10);
    }
    end_test_measurment();
}

bool encrypt_new_payload()
{
    packet_send_counter++;
    uint32_t PAYLOAD_LEN = TEST_PAYLOAD_BYTES_LEN;
    if (TEST_PAYLOAD_BYTES_LEN == RANDOM_SIZE)
    {
        int random_val = esp_random() % 3;
        if (random_val == 0)
        {
            PAYLOAD_LEN = PAYLOAD_4_BYTES;
        }
        else if (random_val == 1)
        {
            PAYLOAD_LEN = PAYLOAD_10_BYTES;
        }
        else if (random_val == 2)
        {
            PAYLOAD_LEN = PAYLOAD_16_BYTES;
        }

        test_payload_buffer_ptr = get_test_payload_buffer(PAYLOAD_LEN);
    }

    uint8_t payload[PAYLOAD_16_BYTES] = {0};
    // esp_fill_random(payload, TEST_PAYLOAD_BYTES_LEN);
    memcpy(payload, test_payload_buffer_ptr, PAYLOAD_LEN);
    beacon_pdu_data pdu = {0};
    fill_marker_in_pdu(&pdu);
    int encrypt_status = encrypt_payload(payload, PAYLOAD_LEN, &pdu);
    if (encrypt_status != 0)
    {
        ESP_LOGE(SENDER_APP_LOG_GROUP, "Failed to encrypt payload, error code: %d", encrypt_status);
        return false;
    }
    
    if ( TEST_ADV_INTERVAL == INT_RANDOM && prev_key_id != get_current_key_id())
    {
        stop_broadcasting();
        current_random_interval_ms = get_time_interval_for_current_session_key();
        default_ble_adv_params.adv_int_min = MS_TO_N_CONVERTION(current_random_interval_ms);
        default_ble_adv_params.adv_int_max = MS_TO_N_CONVERTION(current_random_interval_ms);
        ESP_LOGI(SENDER_APP_LOG_GROUP, "New interval time %lu ms", current_random_interval_ms);
        while (get_broadcast_state() != BROADCAST_CONTROLLER_BROADCASTING_NOT_RUNNING)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        set_broadcasting_payload((uint8_t *)&pdu, get_beacon_pdu_data_len(&pdu));
        start_broadcasting(&default_ble_adv_params);
        prev_key_id = get_current_key_id();
    }
    else
    {
        set_broadcasting_payload((uint8_t *)&pdu, get_beacon_pdu_data_len(&pdu));
    }

    test_log_packet_send(pdu.payload, pdu.payload_size, NULL);
    return true;
}

bool encrypt_new_key_fragment()
{
    packet_send_counter++;
    beacon_key_pdu_data pdu = {0};
    fill_marker_in_key_pdu(&pdu);

    int encrypt_status = get_key_fragment_pdu(&pdu);
    if (encrypt_status != 0)
    {
        ESP_LOGE(SENDER_APP_LOG_GROUP, "Failed to get key fragment, error code: %d", encrypt_status);
        return false;
    }
    
    if ( TEST_ADV_INTERVAL == INT_RANDOM && prev_key_id != get_current_key_id())
    {
        stop_broadcasting();
        current_random_interval_ms = get_time_interval_for_current_session_key();
        default_ble_adv_params.adv_int_min = MS_TO_N_CONVERTION(current_random_interval_ms);
        default_ble_adv_params.adv_int_max = MS_TO_N_CONVERTION(current_random_interval_ms);
        ESP_LOGI(SENDER_APP_LOG_GROUP, "New interval time %lu ms", current_random_interval_ms);
        while (get_broadcast_state() != BROADCAST_CONTROLLER_BROADCASTING_NOT_RUNNING)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        set_broadcasting_payload((uint8_t *)&pdu, get_beacon_key_pdu_data_len());
        start_broadcasting(&default_ble_adv_params);
        prev_key_id = get_current_key_id();
    }
    else
    {
        set_broadcasting_payload((uint8_t *)&pdu, get_beacon_key_pdu_data_len());
    }

    test_log_key_fragment_send();
    return true;
}