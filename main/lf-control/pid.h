#pragma once

template<class Tnum>
Tnum clamp(Tnum val, Tnum limit) {
    if (val > limit) {
        return limit;
    } else if (val < -limit) {
        return -limit;
    } else {
        return val;
    }
}

struct PID_settings_t {
    float kp = 0, ki = 0, kd = 0, ad = .1, ilimit = 1., dlimit = 1.;
};

struct PID_state_t {
    float i = 0, di = 0, prev = 0;
};

float pid_step(const PID_settings_t& settings, PID_state_t* state, float err, float dt);
