idf_component_register(SRCS "./src/beacon_pdu/beacon_pdu_data.c"
                            "./src/crypto/crypto.c"
                            "./src/ble_common/ble_common.c"
                            "./src/ble_send_pdu_counter/ble_send_pdu_counter.c"
                    INCLUDE_DIRS
                            "./include"
                            "./include/beacon_pdu"
                            "./include/ble_common"
                            "./include/crypto"
                            "./include/ble_send_pdu_counter"
                    REQUIRES esp_timer
                    REQUIRES nvs_flash
                    REQUIRES bt
                    REQUIRES mbedtls
                    REQUIRES esp_system
                )
