#include "console.h"
#include "player.h"

#include "freertos/FreeRTOS.h"

extern "C" void app_main();

void app_main() {
    Player::getInstance().start();

    start_console();
}