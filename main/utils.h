#pragma once

#include <cstdint>

inline uint16_t swap_bytes(uint16_t data) {
    return (data << 8) | (data >> 8);
}
