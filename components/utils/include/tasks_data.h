#ifndef TASK_DATA_HPP
#define TASK_DATA_HPP

#include <stdint.h>

typedef struct {
    const char *name;
    uint32_t stackSize;
    uint32_t priority;
    uint32_t core;
} taskData;

typedef enum {
    BLE_CONTROLLER_TASK,
    SEC_PDU_PROCESSING,
    KEY_RECONSTRUCTION_TASK,
    PC_SERIAL_COMMUNICATION_TASK,
    ADV_TIME_AUTHORIZE_TASK
} KNOWN_TASKS;

extern taskData tasksDataArr[];

#endif