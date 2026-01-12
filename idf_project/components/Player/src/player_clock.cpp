#include "player_clock.h"

PlayerMetronome::PlayerMetronome() {
    timer = NULL;
}

PlayerMetronome::~PlayerMetronome() {}

static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
    TaskHandle_t task = (TaskHandle_t)user_ctx;
    xTaskNotify(task, NOTIFICATION_UPDATE, eSetBits);

    return pdFALSE;
}

esp_err_t PlayerMetronome::init(TaskHandle_t _task, uint32_t _period_us) {
    task = _task;
    period_us = _period_us;

    gptimer_config_t timer_config = {.clk_src = GPTIMER_CLK_SRC_DEFAULT,  // Select the default clock source
                                     .direction = GPTIMER_COUNT_UP,       // Counting direction is up
                                     .resolution_hz = 1 * 1000 * 1000,    // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond

                                     .intr_priority = 0,
                                     .flags = {.intr_shared = 0, .allow_pd = 0, .backup_before_sleep = 0}};

    gptimer_new_timer(&timer_config, &timer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,  // Call the user callback function when the alarm event occurs
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, task));

    gptimer_alarm_config_t alarm_config;
    alarm_config.reload_count = 0;
    alarm_config.alarm_count = period_us;
    alarm_config.flags.auto_reload_on_alarm = true;

    gptimer_set_alarm_action(timer, &alarm_config);

    return ESP_OK;
}

esp_err_t PlayerMetronome::deinit() {
    if(running) {
        return ESP_ERR_INVALID_STATE;
    }

    if(timer) {
        gptimer_del_timer(timer);
        timer = NULL;
    }

    task = NULL;
    period_us = 0;

    return ESP_OK;
}

esp_err_t PlayerMetronome::start() {
    if(running) {
        return ESP_OK;
    }
    gptimer_enable(timer);
    gptimer_start(timer);
    running = true;

    return ESP_OK;
}

esp_err_t PlayerMetronome::stop() {
    if(!running) {
        return ESP_OK;
    }

    gptimer_stop(timer);
    gptimer_disable(timer);

    running = false;

    return ESP_OK;
}

esp_err_t PlayerMetronome::reset() {
    if(running) {
        return ESP_ERR_INVALID_STATE;
    }
    gptimer_set_raw_count(timer, 0);

    return ESP_OK;
}

esp_err_t PlayerMetronome::set_period_us(uint32_t new_period_us) {
    if(running) {
        return ESP_ERR_INVALID_STATE;
    }

    period_us = new_period_us;

    gptimer_alarm_config_t alarm_config;
    alarm_config.reload_count = 0;
    alarm_config.alarm_count = period_us;
    alarm_config.flags.auto_reload_on_alarm = true;

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

    return ESP_OK;
}

PlayerClock::PlayerClock() {}
PlayerClock::~PlayerClock() {}

esp_err_t PlayerClock::init(bool _with_metronome, TaskHandle_t task, uint32_t period_us) {
    with_metronome = _with_metronome;
    accumulated_us = 0;
    last_start_us = 0;
    is_playing = false;

    if(with_metronome) {
        metronome.init(task, period_us);
    }

    initialized = true;

    return ESP_OK;
}

esp_err_t PlayerClock::deinit() {
    if(!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if(is_playing) {
        return ESP_ERR_INVALID_STATE;
    }

    metronome.deinit();
    accumulated_us = 0;
    last_start_us = 0;

    initialized = false;

    return ESP_OK;
}

esp_err_t PlayerClock::start() {
    if(!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if(is_playing) {
        return ESP_OK;
    }

    last_start_us = esp_timer_get_time();
    is_playing = true;

    if(with_metronome) {
        metronome.start();
    }

    return ESP_OK;
}

esp_err_t PlayerClock::pause() {
    if(!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if(!is_playing)
        return ESP_OK;

    int64_t now = esp_timer_get_time();
    accumulated_us += (now - last_start_us);
    is_playing = false;

    if(with_metronome) {
        metronome.stop();
    }

    return ESP_OK;
}

esp_err_t PlayerClock::reset() {
    if(!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if(is_playing)
        return ESP_ERR_INVALID_STATE;

    accumulated_us = 0;
    last_start_us = 0;

    if(with_metronome) {
        metronome.reset();
    }

    return ESP_OK;
}

int64_t PlayerClock::now_us() const {
    if(is_playing) {
        return accumulated_us + (esp_timer_get_time() - last_start_us);
    }
    return accumulated_us;
}
