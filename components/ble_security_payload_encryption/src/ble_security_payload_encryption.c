#include "ble_security_payload_encryption.h"
#include "crypto.h"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "test.h"

#define KEY_REPLACEMENT_TIMEOUT_S 30
#define KEY_REPLACEMENT_TIMEOUT_US(s) ((s) * (1000000))

static const char* MSG_SENDER_LOG_GROUP = "MSG_ENCRYPTOR";
static key_128b pre_shared_key;
static key_splitted splitted_pre_shared_key;
static key_128b next_pre_shared_key; 
static key_splitted next_splitted_pre_shared_key;
static volatile bool is_key_replace_request_active = false;
static uint16_t key_id;
static uint8_t key_time_interval_multiplier;
static uint8_t encrypt_payload_arr[MAX_PDU_PAYLOAD_SIZE] = {0};
static  esp_timer_handle_t key_replacement_timer;
void key_replacement_cb();

static volatile uint16_t KEY_REPLACE_TIME_IN_S;
static volatile uint32_t key_replacement_packet_counter = 200;
static volatile uint64_t encrypted_packet_counter = 0;


uint16_t get_current_key_id()
{
    return key_id;
}

uint32_t get_time_interval_for_current_session_key()
{
    return get_adv_interval_from_key_id(get_current_key_id());
}

void key_replacement_cb()
{
    generate_128b_key(&next_pre_shared_key);
    ESP_LOG_BUFFER_HEX("New key: ", next_pre_shared_key.key, sizeof(next_pre_shared_key));
    split_128b_key_to_fragment(&next_pre_shared_key, &next_splitted_pre_shared_key);
    is_key_replace_request_active = true;
    test_log_sender_key_replace_time_in_s(KEY_REPLACEMENT_TIMEOUT_US(KEY_REPLACE_TIME_IN_S));
    memcpy(&pre_shared_key, &next_pre_shared_key, sizeof(pre_shared_key));
    memcpy(&splitted_pre_shared_key, &next_splitted_pre_shared_key, sizeof(pre_shared_key));
}

uint16_t get_random_key_id() {
    return esp_random() & 0x3FFF;  // Mask to ensure range [0x0000, 0x3FFF]
}

uint16_t get_next_key_fragment()
{
    static uint16_t key_fragment_no = 0;
    uint16_t return_fragment_no = key_fragment_no;
    
    key_fragment_no++;
    if (key_fragment_no > 3)
    {
        key_fragment_no = 0;
    }

    return return_fragment_no; 
}

uint8_t get_random_time_interval_value()
{
    return (esp_random() % MAX_TIME_INTERVAL_MULTIPLIER);
}

bool init_payload_encryption()
{
    static bool isInitialized = false;
    if (isInitialized == false)
    {
        key_id = get_random_key_id();
        key_time_interval_multiplier = get_random_time_interval_value();
        generate_128b_key(&pre_shared_key);
        ESP_LOG_BUFFER_HEX("New key: ", next_pre_shared_key.key, sizeof(pre_shared_key));
        split_128b_key_to_fragment(&pre_shared_key, &splitted_pre_shared_key);
    }

    return isInitialized;
}

bool set_key_replacement_pdu_count(const uint32_t count)
{
    key_replacement_packet_counter = count;
    return true;
}

int encrypt_payload(uint8_t * payload, size_t payload_size, beacon_pdu_data * encrypted_pdu)
{
    encrypted_packet_counter++;
    
    if (payload_size > MAX_PDU_PAYLOAD_SIZE)
    {
        ESP_LOGE(MSG_SENDER_LOG_GROUP, "Payload size exceeds maximum allowed size");
        return 1;
    }

    if (encrypted_packet_counter % key_replacement_packet_counter == 0)
    {
        ESP_LOGI(MSG_SENDER_LOG_GROUP, "Key replacement in progress...");
        key_replacement_cb();
        key_id = get_random_key_id();
        encrypted_packet_counter = 0;
    }

    const uint8_t random_xor_seed = get_random_seed();
    uint8_t nonce[NONCE_SIZE] = {0};

    uint16_t pdu_key_session_data = produce_key_session_data(key_id, 0);
    encrypted_pdu->key_session_data = pdu_key_session_data;
    encrypted_pdu->xor_seed = random_xor_seed;
    encrypted_pdu->pdu_no = encrypted_packet_counter;
    encrypted_pdu->cmd = DATA_CMD;

    build_nonce(nonce, &(encrypted_pdu->marker), pdu_key_session_data, random_xor_seed);

    uint8_t encrypt_payload_arr[MAX_PDU_PAYLOAD_SIZE] = {0};  // Local buffer for encryption
    memcpy(encrypt_payload_arr, payload, payload_size);

    uint8_t encrypted_payload[MAX_PDU_PAYLOAD_SIZE] = {0};  // Output buffer
    aes_ctr_encrypt_payload(encrypt_payload_arr, payload_size, pre_shared_key.key, nonce, encrypted_payload);

    memcpy(encrypted_pdu->payload, encrypted_payload, payload_size);
    encrypted_pdu->payload_size = payload_size;

    return 0;
}

int get_key_fragment_pdu(beacon_key_pdu_data * key_pdu)
{
    encrypted_packet_counter++;

    if (encrypted_packet_counter % key_replacement_packet_counter == 0)
    {
        ESP_LOGI(MSG_SENDER_LOG_GROUP, "Key replacement in progress...");
        key_replacement_cb();
        key_id = get_random_key_id();
        encrypted_packet_counter = 0;
    }

    const uint8_t key_fragment_no = get_next_key_fragment();
    const uint8_t random_xor_seed = get_random_seed();
    uint8_t nonce[NONCE_SIZE] = {0};

    uint16_t pdu_key_session_data = produce_key_session_data(key_id, key_fragment_no);
    key_pdu->bcd.key_session_data = pdu_key_session_data;
    key_pdu->bcd.xor_seed = random_xor_seed;
    key_pdu->pdu_no = encrypted_packet_counter;
    key_pdu->cmd = KEY_FRAGMENT_CMD;

    xor_encrypt_key_fragment(splitted_pre_shared_key.fragment[key_fragment_no], key_pdu->bcd.enc_key_fragment, random_xor_seed);
        
    calculate_hmac_of_fragment(splitted_pre_shared_key.fragment[key_fragment_no], key_pdu->bcd.enc_key_fragment, key_pdu->bcd.key_fragment_hmac);

    build_nonce(nonce, &(key_pdu->marker), pdu_key_session_data, random_xor_seed);

    return 0;
}


