#include "pid.h"

float pid_step(const PID_settings_t& settings, PID_state_t* state, float err, float dt) {
    state->i += clamp(err * dt, settings.ilimit);
    state->di += (err - state->prev) / dt;
    state->di -= state->di * settings.ad * dt;
    state->di = clamp(state->di, settings.dlimit);
    state->prev = err;
    return settings.kp * err + settings.ki * state->i + settings.kd * state->di;
}
