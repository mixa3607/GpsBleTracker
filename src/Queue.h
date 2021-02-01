#pragma once

#include <freertos/queue.h>

void test(const QueueHandle_t queue, void* const value){
    unsigned int inQueue = uxQueueMessagesWaiting(queue);
    xQueueIsQueueFullFromISR
}