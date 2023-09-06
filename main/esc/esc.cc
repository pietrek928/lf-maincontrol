#include "esc.h"

/* Obdługa PWM dla ESC */

constexpr static ledc_timer_config_t esc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,     // timer mode
    .duty_resolution = LEDC_TIMER_14_BIT,  // resolution of PWM duty
    .timer_num = LEDC_TIMER_2,             // timer index
    .freq_hz = 50,                         // frequency of PWM signal
    .clk_cfg = LEDC_AUTO_CLK};

constexpr static ledc_channel_config_t esc_config = {
    .gpio_num = 33,
    .speed_mode = esc_timer.speed_mode,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = esc_timer.timer_num,
    .duty = 0,
    .hpoint = 0,
    .flags =
        {
            .output_invert = 0,
        },
};

TaskHandle_t esc_task_handle = NULL;

/* Init */
void esc_init()
{
    ledc_timer_config(&esc_timer);
    ledc_channel_config(&esc_config);

    /* Kalibracja esc */
    escDuty(0.1f);
    vTaskDelay(pdMS_TO_TICKS(2000));
    escDuty(0.05f);
    vTaskDelay(pdMS_TO_TICKS(1000));
}

/** Main ESC task */
void esc_test_task(void* pvParameters)
{
    esc_init();

    // float speed = 0.0f;
    while (1)
    {
        // if (speed < 1.0f) speed += 0.1f;
        escSpeed(0.1f);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

/* Set duty cycle for PWM*/
void escDuty(float duty)
{
    ledc_set_duty(
        esc_timer.speed_mode,
        esc_config.channel,
        floor((uint32_t)16383 * duty));
    ledc_update_duty(esc_timer.speed_mode, esc_config.channel);
}

/* Control the motor speed in 0 - 1.0f range */
void escSpeed(float speed) { escDuty(0.05f + (0.05f * speed)); }
