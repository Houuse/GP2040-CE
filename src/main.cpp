/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

// Pi Pico includes
#include "pico/multicore.h"

// GP2040 includes
#include "gp2040.h"
#include "gp2040aux.h"
#include "bluepad32_platform.h"

#include <cstdlib>

// Custom implementation of __gnu_cxx::__verbose_terminate_handler() to reduce binary size
namespace __gnu_cxx {
void __verbose_terminate_handler()
{
	abort();
}
}

static GP2040 * gp2040Core0 = nullptr;
static GP2040Aux * gp2040Core1 = nullptr;

// Core 1 belongs to the Bluetooth stack. BTstack's run loop blocks forever, which
// is what it is designed to do when it owns a core (same design as PicoSwitch).
// This replaces GP2040Aux: display and LED addons are given up for Bluetooth.
void core1() {
	multicore_lockout_victim_init(); // block core 1
	gp2040_bluepad32_core1();        // does not return
}

int main() {
	// Create GP2040 Main Core (core0)
	gp2040Core0 = new GP2040();

	// Create GP2040 Main Core - Setup Core0
	gp2040Core0->setup();

	// Hand core 1 to Bluetooth
	multicore_launch_core1(core1);

	// Core 0 does not wait for Bluetooth: USB must enumerate and pass PS4 auth
	// whether or not a controller has paired yet.
	gp2040Core0->run();

	return 0;
}
