#ifndef BEACON_PDU_DATA_H
#define BEACON_PDU_DATA_H

#include <esp_err.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "beacon_crypto_data.h"
#include "beacon_marker.h"

typedef uint8_t command;

#define MIN_ADV_TIME_MS 3000
#define MAX_ADV_TIME_MS 5000
#define SCALE_SINGLE_MS 80

#define MAX_GAP_DATA_LEN 31
#define MAX_PDU_PAYLOAD_SIZE (MAX_GAP_DATA_LEN - (sizeof(uint16_t)) - (MARKER_STRUCT_SIZE) - (sizeof(command) - sizeof(uint16_t)))
#define NONCE_SIZE 16

#define DATA_CMD 1
#define KEY_FRAGMENT_CMD 2

#define COMMAND_OFFSET sizeof(beacon_marker)
#define PDU_NO_OFFSET ((sizeof(beacon_marker)) + (sizeof(command)))
#define KEY_SESSION_OFFSET ((PDU_NO_OFFSET) +  (sizeof(uint16_t)))

typedef struct {
    beacon_marker marker;
    command cmd;
    uint16_t pdu_no;
    uint16_t key_session_data;
    uint8_t xor_seed;
    uint8_t payload[MAX_PDU_PAYLOAD_SIZE];
    size_t payload_size;
}__attribute__((packed)) beacon_pdu_data;


typedef struct {
    beacon_marker marker;
    command cmd;
    uint16_t pdu_no;
    beacon_crypto_data bcd;
}__attribute__((packed)) beacon_key_pdu_data;


size_t get_beacon_pdu_data_len(beacon_pdu_data * pdu);

size_t get_beacon_key_pdu_data_len();

size_t get_payload_size_from_pdu(size_t total_pdu_len);

bool get_beacon_pdu_from_adv_data(beacon_pdu_data * pdu, uint8_t *data, size_t size);

command get_command_from_pdu(uint8_t *data, size_t size); 

esp_err_t build_beacon_pdu_data (uint16_t key_session_data, uint8_t* payload, size_t payload_size, beacon_pdu_data *bpd);

esp_err_t build_beacon_key_pdu_data (beacon_crypto_data* bcd, beacon_key_pdu_data *bpd);

esp_err_t fill_marker_in_pdu(beacon_pdu_data *bpd);

esp_err_t fill_marker_in_key_pdu(beacon_key_pdu_data *bpd);

bool is_pdu_in_beacon_pdu_format(uint8_t *data, size_t size);

void build_nonce(uint8_t nonce[NONCE_SIZE], const beacon_marker* marker, uint16_t key_session_data, uint8_t xor_seed);

uint16_t produce_key_session_data(uint16_t key_id, uint8_t key_fragment);

uint8_t produce_key_exchange_data(uint8_t pdu_time_interval_ms, uint8_t key_exchange_counter);

uint16_t get_key_id_from_key_session_data(uint16_t session_data);

uint8_t get_key_fragment_index_from_key_session_data(uint16_t session_data);

uint16_t get_key_expected_time_interval_ms(uint8_t key_exchange_data);

uint8_t get_key_exchange_counter(uint8_t key_exchange_data);

uint8_t get_key_expected_time_interval_multiplier(uint8_t key_exchange_data);

uint32_t get_adv_interval_from_key_id(uint16_t key_id);

#endif