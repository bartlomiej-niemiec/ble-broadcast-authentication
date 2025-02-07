#include "beacon_pdu/beacon_pdu_data.h"
#include <string.h>
#include "esp_log.h"

static const char* BEACON_PDU_GROUP = "BEACON_PDU_GROUP";

beacon_marker my_marker = {
    .marker = {0xFF, 0x8, 0x0}
};

esp_err_t build_beacon_pdu_data (beacon_crypto_data *crypto, uint8_t* payload, size_t payload_size, beacon_pdu_data *bpd)
{
    if ((crypto == NULL) || (payload == NULL || payload_size > MAX_PDU_PAYLOAD_SIZE) || (bpd == NULL)){
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&(bpd->marker), &my_marker, sizeof(beacon_marker));
    memcpy(&(bpd->bcd), crypto, sizeof(beacon_crypto_data));
    memcpy(&(bpd->payload), payload, payload_size);

    return ESP_OK;
}

bool is_pdu_in_beacon_pdu_format(uint8_t *data, size_t size) 
{
    bool is_pdu_beacon_format = false;

    if (size < sizeof(beacon_marker))
    {
        ESP_LOGE(BEACON_PDU_GROUP, "PDU size is lower than marker");
    }
    else
    {
        is_pdu_beacon_format = memcmp(data, &my_marker, sizeof(beacon_marker)) == 0;
    }

    return is_pdu_beacon_format;
}

esp_err_t fill_marker_in_pdu(beacon_pdu_data *bpd)
{
    if ((bpd == NULL)){
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&(bpd->marker), &my_marker, sizeof(beacon_marker));
    return ESP_OK;
}

void build_nonce(uint8_t nonce[NONCE_SIZE], const beacon_marker* marker, uint16_t key_session_data, uint8_t xor_seed)
{
    memset(nonce, 0, 16);

    // Copy the marker into the beginning of the nonce.
    memcpy(nonce, marker->marker, sizeof(beacon_marker));

    // Add the key ID (2 bytes) in little-endian format.
    nonce[sizeof(beacon_marker)]     = key_session_data & 0xFF;        // Low byte of key ID
    nonce[sizeof(beacon_marker) + 1] = (key_session_data >> 8) & 0xFF; // High byte of key ID

    // Add the XOR seed (1 byte).
    nonce[sizeof(beacon_marker) + 2] = xor_seed;

    // The remaining bytes of the nonce are already zero-filled from memset.
}

uint16_t get_key_id_from_key_session_data(uint16_t session_data)
{
    static const uint16_t MASK = 0x3FFF;
    return session_data & MASK;
}

uint8_t get_key_fragment_index_from_key_session_data(uint16_t session_data)
{
    return (session_data >> 14) & 0x03;
}

uint16_t get_key_expected_time_interval_ms(uint8_t key_exchange_data)
{
    return ((key_exchange_data >> 4) & 0xF) * PDU_INTERVAL_RESOLUTION_MS;
}

uint8_t get_key_expected_time_interval_multiplier(uint8_t key_exchange_data)
{
    return ((key_exchange_data >> 4) & 0xF);
}

uint8_t get_key_exchange_counter(uint8_t key_exchange_data)
{
    return (key_exchange_data & 0xF);
}

size_t get_beacon_pdu_data_len(beacon_pdu_data * pdu)
{
    return (sizeof(pdu->bcd) + sizeof(pdu->marker) + pdu->payload_size);
}

uint16_t produce_key_session_data(uint16_t key_id, uint8_t key_fragment)
{
    if (key_fragment > 3)
    {
        ESP_LOGE(BEACON_PDU_GROUP, "Key fragment should in range 0-3");
        return 0;
    }


    key_fragment = key_fragment & 0x03; // Mask to keep only the lower 2 bits
    key_id = key_id & 0x3FFF;           // Mask to keep only the lower 14 bits

    // Combine the values
    uint16_t result = (key_fragment << 14) | key_id;

    return result;
}

uint8_t produce_key_exchange_data(uint8_t pdu_time_interval_ms, uint8_t key_exchange_counter)
{
    return ((pdu_time_interval_ms << 4) | key_exchange_counter);
}

size_t get_payload_size_from_pdu(size_t total_pdu_len)
{
    return (total_pdu_len - (CRYPT_DATA_SIZE + MARKER_STRUCT_SIZE));
}