#include <fcntl.h>
#include <stdio.h>
#include "player.h"

#include "freertos/FreeRTOS.h"

extern "C" void app_main();

void app_main() {
    Player& player = Player::getInstance();
    player.init();
    vTaskDelay(pdMS_TO_TICKS(1000));

    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

    printf("CMD: 0=Play, 1=Pause, 2=Test, 3=Reset, 4=Kill\n");

    while(1) {
        int c = getchar();

        if(c != EOF && c != '\n') {

            Event event;
            bool valid = true;

            switch(c) {
                case '0':
                    event.type = EVENT_PLAY;
                    break;
                case '1':
                    event.type = EVENT_PAUSE;
                    break;
                case '2':
                    event.type = EVENT_TEST;
                    break;
                case '3':
                    event.type = EVENT_RESET;
                    break;
                case '4':
                    event.type = EVENT_KILL;
                    break;
                default:
                    valid = false;
                    break;
            }

            if(valid) {
                printf("Sent CMD: %c\n", (char)c);
                xQueueSend(player.eventQueue, &event, 1000);
            }
        }

        if(c == '4') {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}