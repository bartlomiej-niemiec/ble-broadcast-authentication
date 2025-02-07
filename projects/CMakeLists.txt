# Set default APP_TARGET if not defined
if(NOT DEFINED CONFIG_APP_TARGET)
    set(CONFIG_APP_TARGET "receiver_app")
endif()

# Print the selected APP_TARGET for debugging
message(STATUS "Selected APP_TARGET: ${CONFIG_APP_TARGET}")

# set(srcs)
# set(incl)
# set(req)

# # Register components based on APP_TARGET
# if(CONFIG_APP_TARGET STREQUAL "receiver_app")
#     list(APPEND srcs "./receiver_app/receiver_main.c" "./receiver_app/ble_gap_payload_consuming.c")
#     list(APPEND incl "./receiver_app")
#     list(APPEND req "ble_broadcast_security_processing_engine" "core" "utils" "ble_broadcast_controller")
# elseif(CONFIG_APP_TARGET STREQUAL "sender_app")
#     list(APPEND srcs "./sender_app/sender_main.c")
#     list(APPEND incl "./sender_app")
#     list(APPEND req "ble_security_payload_encryption" "core" "ble_broadcast_controller")
# else()
#     message(FATAL_ERROR "Invalid APP_TARGET: ${CONFIG_APP_TARGET}. Must be 'receiver_app' or 'sender_app'.")
# endif()

idf_component_register(
        SRCS "./receiver_app/receiver_main.c"
        INCLUDE_DIRS "./receiver_app"
        REQUIRES "ble_broadcast_security_processing_engine" "core" "utils" "ble_broadcast_controller" "test_framework" 
    )

# idf_component_register(
#     SRCS "./sender_app/sender_main.c"
#     INCLUDE_DIRS "./sender_app"
#     REQUIRES "ble_security_payload_encryption" "core" "ble_broadcast_controller" "pc_communication_serial" "test_framework"
#     REQUIRES esp_timer
# )

# idf_component_register(
#     SRCS "./sender_app_2nd/sender_main.c"
#     INCLUDE_DIRS "./sender_app_2nd"
#     REQUIRES "ble_security_payload_encryption" "core" "ble_broadcast_controller" "pc_communication_serial" "test_framework"
#     REQUIRES esp_timer
# )


