#include <stdio.h>
#include "freertos/FreeRTOS.h"




void app_main(void)
{
    printf("hello\n");
    while (true) {
        printf("hello\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
