#ifndef BLE_PDU_ENGINE_ENCRYPTOR_H
#define BLE_PDU_ENGINE_ENCRYPTOR_H

#include "beacon_pdu_data.h"
#include "stddef.h"

bool init_payload_encryption();

bool set_key_replacement_pdu_count(const uint32_t count);

int encrypt_payload(uint8_t * payload, size_t payload_size, beacon_pdu_data * encrypted_pdu);

int get_key_fragment_pdu(beacon_key_pdu_data * key_pdu);

uint32_t get_time_interval_for_current_session_key();

uint16_t get_current_key_id();

#endif