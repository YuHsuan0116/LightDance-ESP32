#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "fsm.h"

fsm_handle_t fsm;

event_t events[EVENT_COUNT] = {
    EVENT_STOP,
    EVENT_PAUSE,
    EVENT_PLAY,
    EVENT_START,
    EVENT_PART_TEST,
    EVENT_UPDATE_FRAME,
};

void event_transmitter(void* pvParameters) {
    xQueueSend(fsm.event_queue, &events[3], 0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    xQueueSend(fsm.event_queue, &events[2], 0);
    vTaskDelay(pdMS_TO_TICKS(5000));

    xQueueSend(fsm.event_queue, &events[1], 0);
    vTaskDelay(pdMS_TO_TICKS(5000));

    xQueueSend(fsm.event_queue, &events[2], 0);
    vTaskDelay(pdMS_TO_TICKS(5000));

    xQueueSend(fsm.event_queue, &events[0], 0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    xQueueSend(fsm.event_queue, &events[3], 0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    xQueueSend(fsm.event_queue, &events[4], 0);
    vTaskDelay(pdMS_TO_TICKS(5000));

    xQueueSend(fsm.event_queue, &events[0], 0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    while(true) {
        int idx = rand() % (EVENT_COUNT);
        xQueueSend(fsm.event_queue, &events[idx], 0);
        ESP_LOGI("event_transmitter_task", "send event: %s", fsm_getEventName(events[idx]));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

void fsm_task(void* pvParameters) {
    fsm_init(&fsm);
    while(true) {
        event_t received_event;
        if(xQueueReceive(fsm.event_queue, &received_event, pdMS_TO_TICKS(100))) {
            ESP_LOGI("fsm_task", "receive event: %s", fsm_getEventName(received_event));
            fsm_transit(received_event, &fsm);
        }
    }

    vTaskDelete(NULL);
}

void app_main(void) {
    srand(time(NULL));

    xTaskCreate(fsm_task, "fsm_task", 2048, NULL, 5, NULL);
    xTaskCreate(event_transmitter, "event_transmitter_task", 2048, NULL, 5, NULL);
}