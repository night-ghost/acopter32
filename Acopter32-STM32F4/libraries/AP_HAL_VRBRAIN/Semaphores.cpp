/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_VRBRAIN

#include <AP_HAL_VRBRAIN.h>
#include "Semaphores.h"
#include "Scheduler.h"

using namespace VRBRAIN;

extern const AP_HAL::HAL& hal;

// Constructor
VRBRAINSemaphore::VRBRAINSemaphore() : _taken(false) {}

bool VRBRAINSemaphore::give() {
    if (!_taken) {
        return false;
    } else {
        _taken = false;
        return true;
    }
}

bool VRBRAINSemaphore::take(uint32_t timeout_ms) {
    if (hal.scheduler->in_timerprocess()) {
        hal.scheduler->panic(PSTR("PANIC: AVRSemaphore::take used from "
                    "inside timer process"));
        return false; /* Never reached - panic does not return */
    }
    return _take_from_mainloop(timeout_ms);
}

bool VRBRAINSemaphore::take_nonblocking() {
    if (hal.scheduler->in_timerprocess()) {
        return _take_nonblocking();
    } else {
        return _take_from_mainloop(0);
    }
}

bool VRBRAINSemaphore::_take_from_mainloop(uint32_t timeout_ms) {
    /* Try to take immediately */
    if (_take_nonblocking()) {
        return true;
    } else if (timeout_ms == 0) {
        /* Return immediately if timeout is 0 */
        return false;
    }

    uint16_t timeout_ticks = timeout_ms*10;
    do {
        /* Delay 1ms until we can successfully take, or we timed out */
        hal.scheduler->delay_microseconds(100);
        timeout_ticks--;
        if (_take_nonblocking()) {
            return true;
        }
    } while (timeout_ticks > 0);

    return false;
}

bool VRBRAINSemaphore::_take_nonblocking() {
    bool result = false;
    noInterrupts();
    if (!_taken) {
        _taken = true;
        result = true;
    }
    interrupts();
    return result;
}

#endif
