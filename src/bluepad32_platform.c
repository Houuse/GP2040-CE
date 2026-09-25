// Bluepad32 platform for GP2040-CE.
// Milestone 2: prove both stacks coexist. Logs over UART; USB is the DS4.

#include <stdio.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <btstack_run_loop.h>
#include <uni.h>

#include "sdkconfig.h"
#include "bluepad32_platform.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Shared state, written here and read by the GP2040-CE addon (milestone 3).
volatile struct bp32_shared_state bp32_state;

static void plat_init(int argc, const char** argv) {
    (void)argc; (void)argv;
    printf("[bp32] platform init\n");
}

static void plat_on_init_complete(void) {
    printf("[bp32] init complete, scanning\n");
    uni_bt_start_scanning_and_autoconnect_unsafe();
    uni_bt_allow_incoming_connections(true);
}

static uni_error_t plat_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    (void)addr; (void)cod; (void)rssi;
    printf("[bp32] discovered: %s\n", name ? name : "(no name)");
    return UNI_ERROR_SUCCESS;
}

static void plat_on_device_connected(uni_hid_device_t* d) {
    (void)d;
    printf("[bp32] connected\n");
}

static void plat_on_device_disconnected(uni_hid_device_t* d) {
    (void)d;
    printf("[bp32] disconnected\n");
    bp32_state.connected = false;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);  // no UART here: LED is the signal
}

static uni_error_t plat_on_device_ready(uni_hid_device_t* d) {
    (void)d;
    printf("[bp32] gamepad ready\n");
    bp32_state.connected = true;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);  // solid LED = controller ready
    return UNI_ERROR_SUCCESS;
}

static void plat_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    (void)d;
    if (ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD)
        return;

    const uni_gamepad_t* gp = &ctl->gamepad;

    bp32_state.buttons  = gp->buttons;
    bp32_state.misc     = gp->misc_buttons;
    bp32_state.dpad     = gp->dpad;
    bp32_state.axis_x   = gp->axis_x;
    bp32_state.axis_y   = gp->axis_y;
    bp32_state.axis_rx  = gp->axis_rx;
    bp32_state.axis_ry  = gp->axis_ry;
    bp32_state.brake    = gp->brake;
    bp32_state.throttle = gp->throttle;
    bp32_state.seq++;
}

static const uni_property_t* plat_get_property(uni_property_idx_t idx) {
    (void)idx;
    return NULL;
}

static void plat_on_oob_event(uni_platform_oob_event_t event, void* data) {
    (void)event; (void)data;
}

struct uni_platform* gp2040_get_platform(void) {
    static struct uni_platform plat = {
        .name = "GP2040-CE",
        .init = plat_init,
        .on_init_complete = plat_on_init_complete,
        .on_device_discovered = plat_on_device_discovered,
        .on_device_connected = plat_on_device_connected,
        .on_device_disconnected = plat_on_device_disconnected,
        .on_device_ready = plat_on_device_ready,
        .on_oob_event = plat_on_oob_event,
        .on_controller_data = plat_on_controller_data,
        .get_property = plat_get_property,
    };
    return &plat;
}

// Entry point for core 1. Does not return: BTstack's run loop owns this core.
void gp2040_bluepad32_core1(void) {
    memset((void*)&bp32_state, 0, sizeof(bp32_state));

    if (cyw43_arch_init() != 0) {
        printf("[bp32] cyw43_arch_init FAILED\n");
        return;
    }

    // two quick blinks: cyw43 is alive and BTstack is about to start
    for (int i = 0; i < 2; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(120);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(120);
    }

    uni_platform_set_custom(gp2040_get_platform());
    uni_init(0, NULL);
    printf("[bp32] initialised, entering run loop\n");

    btstack_run_loop_execute();  // blocks; this core is BTstack's
}
