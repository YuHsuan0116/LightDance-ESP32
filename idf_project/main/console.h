#pragma once

#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"

#include "player.h"

#define PROMPT_STR "cmd"

static Event e;
static esp_console_repl_t* repl = NULL;
static esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

static int sendPlay(int argc, char** argv) {
    e.type = EVENT_PLAY;
    Player::getInstance().sendEvent(e);
    return 0;
}

static void register_sendPlay(void) {
    const esp_console_cmd_t cmd = {.command = "play",
                                   .help = "send play",
                                   .hint = NULL,
                                   .func = &sendPlay,

                                   .argtable = NULL,
                                   .func_w_context = NULL,
                                   .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int sendPause(int argc, char** argv) {
    e.type = EVENT_PAUSE;
    Player::getInstance().sendEvent(e);
    return 0;
}

static void register_sendPause(void) {
    const esp_console_cmd_t cmd = {.command = "pause",
                                   .help = "send pause",
                                   .hint = NULL,
                                   .func = &sendPause,

                                   .argtable = NULL,
                                   .func_w_context = NULL,
                                   .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int sendTest(int argc, char** argv) {
    e.type = EVENT_TEST;
    Player::getInstance().sendEvent(e);
    return 0;
}

static void register_sendTest(void) {
    const esp_console_cmd_t cmd = {.command = "test",
                                   .help = "send test",
                                   .hint = NULL,
                                   .func = &sendTest,

                                   .argtable = NULL,
                                   .func_w_context = NULL,
                                   .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int sendReset(int argc, char** argv) {
    e.type = EVENT_RESET;
    Player::getInstance().sendEvent(e);
    return 0;
}

static void register_sendReset(void) {
    const esp_console_cmd_t cmd = {.command = "reset",
                                   .help = "send reset",
                                   .hint = NULL,
                                   .func = &sendReset,

                                   .argtable = NULL,
                                   .func_w_context = NULL,
                                   .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int sendExit(int argc, char** argv) {
    e.type = EVENT_RESET;
    Player::getInstance().sendEvent(e);
    return 0;
}

static void register_sendExit(void) {
    const esp_console_cmd_t cmd = {.command = "exit",
                                   .help = "send exit",
                                   .hint = NULL,
                                   .func = &sendExit,

                                   .argtable = NULL,
                                   .func_w_context = NULL,
                                   .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static int stop_console(int argc, char** argv) {
    esp_console_stop_repl(repl);
    return 0;
}

static void register_stop_console(void) {
    const esp_console_cmd_t cmd = {.command = "stop",
                                   .help = "stop console",
                                   .hint = NULL,
                                   .func = &stop_console,

                                   .argtable = NULL,
                                   .func_w_context = NULL,
                                   .context = NULL};
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void register_cmd() {
    register_sendPlay();
    register_sendPause();
    register_sendReset();
    register_sendExit();
    register_sendTest();
    register_stop_console();
}

void start_console() {

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = PROMPT_STR ">";
    repl_config.max_cmdline_length = 1024;

    esp_console_register_help_command();
    register_cmd();

    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}