# idf_component_register(
#         SRCS "./receiver_app/receiver_main.c"
#         INCLUDE_DIRS "./receiver_app"
#         REQUIRES "ble_broadcast_security_processing_engine" "core" "utils" "ble_broadcast_controller" "test_framework" 
#     )

idf_component_register(
    SRCS "./sender_app/sender_main.c"
    INCLUDE_DIRS "./sender_app"
    REQUIRES "ble_security_payload_encryption" "core" "ble_broadcast_controller" "pc_communication_serial" "test_framework"
    REQUIRES esp_timer
)

# idf_component_register(
#     SRCS "./sender_app_2nd/sender_main.c"
#     INCLUDE_DIRS "./sender_app_2nd"
#     REQUIRES "ble_security_payload_encryption" "core" "ble_broadcast_controller" "pc_communication_serial" "test_framework"
#     REQUIRES esp_timer
# )


