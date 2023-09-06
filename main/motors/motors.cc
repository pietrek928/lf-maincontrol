#include "motors.h"
// #include "Init.h"

#include "driver/mcpwm_prelude.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char TAG[] = "Motor";

#define TIMER_RESOLUTION_HZ 1000000  // 1MHz, 1us per tick
#define TIMER_PERIOD        1000     // 1000 ticks, 1ms
#define PWM1_GPIO20           33//20
#define PWM2_GPIO21           23//21
#define NB 2 // Number of timers

constexpr static gpio_num_t EN_motor = gpio_num_t(21); //LF - 34 PIN

mcpwm_timer_handle_t timers[NB];
mcpwm_cmpr_handle_t comparators[NB];

constexpr static mcpwm_timer_config_t timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = TIMER_RESOLUTION_HZ,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    .period_ticks = TIMER_PERIOD,
    .flags = {
        .update_period_on_empty = false,
        .update_period_on_sync = true,
    }
};

constexpr static mcpwm_operator_config_t operator_config = {
    .group_id = 0, // operator should be in the same group of the above timers
    .flags{}
};

constexpr static mcpwm_comparator_config_t compare_config = {
    .flags = {
        .update_cmp_on_tez = true,
        .update_cmp_on_tep = false,
        .update_cmp_on_sync = false,
    }
};

constexpr static mcpwm_generator_config_t gen_config[NB] = {{
    .gen_gpio_num = PWM1_GPIO20,
    .flags = {
        .invert_pwm = false,
        .io_loop_back = false,
    },
}, {
    .gen_gpio_num = PWM2_GPIO21,
    .flags = {
        .invert_pwm = false,
        .io_loop_back = false,
    }
}};

//          +->timer1
// (TEZ)    |
// timer0---+
//          |
//          +->timer2
//Create TEZ sync source from timer0
constexpr static mcpwm_timer_sync_src_config_t timer_sync_config = {
    .timer_event = MCPWM_TIMER_EVENT_EMPTY, // generate sync event on timer empty
    .flags = {
        .propagate_input_sync = true,
    }
};

void mcpwm_init() {
    ESP_LOGI(TAG,"Initialization of MCPWM");

    /* GPIO for EN Pin */
    // configure_led(EN_motor,GPIO_MODE_OUTPUT);

    for (int i = 0; i < NB; ++i) {
        mcpwm_oper_handle_t operator_;

        ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timers[i]));
        ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &operator_));
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operator_, timers[i]));

        ESP_ERROR_CHECK(mcpwm_new_comparator(operator_, &compare_config, &comparators[i]));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparators[i], 500)); // SET PWM DUTY

        mcpwm_gen_handle_t generator;
        ESP_ERROR_CHECK(mcpwm_new_generator(operator_, &gen_config[i], &generator));

        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_timer_event(
            generator,
            (mcpwm_gen_timer_event_action_t) {
                .direction = MCPWM_TIMER_DIRECTION_UP, .event = MCPWM_TIMER_EVENT_EMPTY, .action = MCPWM_GEN_ACTION_HIGH
            },
            (mcpwm_gen_timer_event_action_t) {
                .direction = MCPWM_TIMER_DIRECTION_UP, .event = MCPWM_TIMER_EVENT_INVALID, .action = MCPWM_GEN_ACTION_KEEP
            }
        ));

        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(
            generator,
            (mcpwm_gen_compare_event_action_t) {
                .direction = MCPWM_TIMER_DIRECTION_UP, .comparator = comparators[i], .action = MCPWM_GEN_ACTION_LOW
            },
            (mcpwm_gen_compare_event_action_t) {
                .direction = MCPWM_TIMER_DIRECTION_DOWN, .comparator = comparators[i], .action = MCPWM_GEN_ACTION_HIGH
            },
            (mcpwm_gen_compare_event_action_t) {
                .direction = MCPWM_TIMER_DIRECTION_DOWN, .comparator = NULL, .action = MCPWM_GEN_ACTION_KEEP
            }
        ));

        /* Starting the PWM */
        ESP_ERROR_CHECK(mcpwm_timer_enable(timers[i]));
        // ESP_ERROR_CHECK(mcpwm_timer_start_stop(timers[i], MCPWM_TIMER_START_NO_STOP));
    }

    /* Synchronisation */
    //Set other timers sync to the first timer
    mcpwm_sync_handle_t timer_sync_source;
    ESP_ERROR_CHECK(mcpwm_new_timer_sync_src(timers[0], &timer_sync_config, &timer_sync_source));

    mcpwm_timer_sync_phase_config_t sync_phase_config = {
        .sync_src = timer_sync_source,
        .count_value = 0,
        .direction = MCPWM_TIMER_DIRECTION_UP,
    };

    for (int i = 0; i < NB; ++i) {
        ESP_ERROR_CHECK(mcpwm_timer_set_phase_on_sync(timers[i], &sync_phase_config));
    }
    mcpwm_stop_motor();
}

/** Check the value in limit*/
float mcpwm_saturation(float val) {
    if (val < 0.01f)return 0.01f;
    if(val > 0.99f) return 0.99f;
    return val;
}

/** Start the motor with 50% duty */
void mcpwm_start_motor(float duty) {
    duty = mcpwm_saturation(duty);

    gpio_set_level(EN_motor, 1);

    mcpwm_timer_start_stop(timers[0], MCPWM_TIMER_START_NO_STOP);
    mcpwm_timer_start_stop(timers[1], MCPWM_TIMER_START_NO_STOP);

    mcpwm_comparator_set_compare_value(comparators[0], 1000 * duty);
    mcpwm_comparator_set_compare_value(comparators[1], 1000 * duty);
}

/** Update the duty of the motor 0.01f - 0.99f range */
void mcpwm_update_motors(float dutyA, float dutyB) {
    dutyA = mcpwm_saturation(dutyA);
    dutyB = mcpwm_saturation(dutyB);

    mcpwm_comparator_set_compare_value(comparators[0], 1000 * dutyA);
    mcpwm_comparator_set_compare_value(comparators[1], 1000 * dutyB);

}

/** Stop the motor */
void mcpwm_stop_motor() {
    gpio_set_level(EN_motor, 0);

    mcpwm_timer_start_stop(timers[0], MCPWM_TIMER_STOP_EMPTY);
    mcpwm_timer_start_stop(timers[1], MCPWM_TIMER_STOP_EMPTY);
}
