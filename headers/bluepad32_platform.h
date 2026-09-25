#ifndef BLUEPAD32_PLATFORM_H_
#define BLUEPAD32_PLATFORM_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Written by the Bluepad32 callback, read by the GP2040-CE input addon.
struct bp32_shared_state {
    uint32_t seq;        // bumps on every controller report
    bool connected;
    uint16_t buttons;    // face/shoulder bits
    uint16_t misc;       // 0x01 SYSTEM(PS), 0x02 SELECT, 0x04 START, 0x08 CAPTURE
    uint8_t dpad;
    int32_t axis_x, axis_y, axis_rx, axis_ry;
    int32_t brake, throttle;
};

extern volatile struct bp32_shared_state bp32_state;

// Entry point for core 1. Does not return.
void gp2040_bluepad32_core1(void);

#ifdef __cplusplus
}
#endif

#endif  // BLUEPAD32_PLATFORM_H_
