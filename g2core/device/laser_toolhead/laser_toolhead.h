/*
 * laser_toolhead.h - toolhead driver for a laser, controlled by spindle commands
 * This file is part of the g2core project
 *
 * Copyright (c) 2020 Robert Giseburt
 * Copyright (c) 2020 Alden S. Hart, Jr.
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software library without
 * restriction. Specifically, if other files instantiate templates or use macros or
 * inline functions from this file, or you compile this file and link it with  other
 * files to produce an executable, this file does not by itself cause the resulting
 * executable to be covered by the GNU General Public License. This exception does not
 * however invalidate any other reasons why the executable file might be covered by the
 * GNU General Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LASER_TOOLHEAD_H_ONCE
#define LASER_TOOLHEAD_H_ONCE

#include "spindle.h"
#include "stepper.h" // for st_request_load_move
#include "util.h" // for fp_NE
#include "safety_manager.h" // for safety_manager

/* A few notes:
 *
 * This module drives a laser output based on the spindle controls.
 *
 * Laser ON/OFF (NOT fire, just "is active") is on the `enable_output` pin,
 * and actual fire/pulse is on the `pwm_output` pin.
 *
 */


// class declaration
// note implementation is after
class LaserTool : public ToolHead {
    spDirection direction;        // direction
    float speed;                  // S in RPM
    // float speed_actual;           // actual speed (during speed ramping)

    float speed_min;              // minimum settable spindle speed
    float speed_max;              // maximum settable spindle speed

    bool paused;                  // true if paused, false is not

    uint8_t pwm_output_num;
    gpioDigitalOutput *pwm_output = nullptr;

    uint8_t enable_output_num;
    gpioDigitalOutput *enable_output = nullptr;

    void complete_change();

   public:
    // constructor - provide it with the default output pins - 0 means no pin
    LaserTool(const uint8_t pwm_pin_number, const uint8_t enable_pin_number);

    // ToolHead overrides
    void init() override;

    void pause() override;          // soft-stop the toolhead (usually for a feedhold) - retain all state for resume
    void resume() override;         // resume from the pause - return STAT_EAGAIN if it's not yet ready
    bool ready_to_resume() override;  // return true if paused and resume would not result in an error
    // bool busy() override;             // return true if motion should continue waiting for this toolhead

    // the result of an S word
    // override this to return false - "don't add a command to the buffer"
    bool set_speed(float) override { return (false); }
    float get_speed() override;

    // the result of an M3/M4/M5
    // override this to return false - "don't add a command to the buffer"
    bool set_direction(spDirection) override { return (false); }
    spDirection get_direction() override;

    void stop() override;

    // called from the loader right before a move, with the gcode model to use
    void engage(const GCodeState_t &gm) override;

    bool is_on() override;  // return if the current direction is anything but OFF, **even if paused**

    void set_pwm_output(const uint8_t pwm_pin_number) override;
    void set_enable_output(const uint8_t enable_pin_number) override;

    void set_frequency(float new_frequency)override;
    float get_frequency() override;

    // trivial getters and setters - inlined
    void set_speed_min(float new_speed_min) override { speed_min = new_speed_min; }
    float get_speed_min() override { return speed_min; }
    void set_speed_max(float new_speed_max) override { speed_max = new_speed_max; }
    float get_speed_max() override { return speed_max; }
    // void set_speed_change_per_tick(float new_speed_change_per_tick) override { speed_change_per_tick = new_speed_change_per_tick; }
    // float get_speed_change_per_tick() override { return speed_change_per_tick; }
    // void set_spinup_delay(float new_spinup_delay) override { spinup_delay = new_spinup_delay; }
    // float get_spinup_delay() override { return spinup_delay; }

    // void set_cw_speed_lo(float new_speed_lo) override { cw.speed_lo = new_speed_lo; }
    // float get_cw_speed_lo() override { return cw.speed_lo; }
    // void set_cw_speed_hi(float new_speed_hi) override { cw.speed_hi = new_speed_hi; }
    // float get_cw_speed_hi() override { return cw.speed_hi; }
    // void set_cw_phase_lo(float new_phase_lo) override { cw.phase_lo = new_phase_lo; }
    // float get_cw_phase_lo() override { return cw.phase_lo; }
    // void set_cw_phase_hi(float new_phase_hi) override { cw.phase_hi = new_phase_hi; }
    // float get_cw_phase_hi() override { return cw.phase_hi; }

    // void set_ccw_speed_lo(float new_speed_lo) override { ccw.speed_lo = new_speed_lo; }
    // float get_ccw_speed_lo() override { return ccw.speed_lo; }
    // void set_ccw_speed_hi(float new_speed_hi) override { ccw.speed_hi = new_speed_hi; }
    // float get_ccw_speed_hi() override { return ccw.speed_hi; }
    // void set_ccw_phase_lo(float new_phase_lo) override { ccw.phase_lo = new_phase_lo; }
    // float get_ccw_phase_lo() override { return ccw.phase_lo; }
    // void set_ccw_phase_hi(float new_phase_hi) override { ccw.phase_hi = new_phase_hi; }
    // float get_ccw_phase_hi() override { return ccw.phase_hi; }

    // void set_phase_off(float new_phase_off) override { phase_off = new_phase_off; }
    // float get_phase_off() override { return phase_off; }
};

// method implementations follow

LaserTool::LaserTool(const uint8_t pwm_pin_number, const uint8_t enable_pin_number)
    : pwm_output_num{pwm_pin_number},
      enable_output_num{enable_pin_number} {}

void LaserTool::init()
{
    // TODO - ensure outputs are within range
    set_pwm_output(pwm_output_num);
    set_enable_output(enable_output_num);
}

void LaserTool::pause() {
    if (paused) {
        return;
    }

    paused = true;
    this->complete_change();
}

void LaserTool::resume() {
    if (!paused) {
        return;
    }

    paused = false;
    this->complete_change();
}

bool LaserTool::ready_to_resume() { return paused && safety_manager->ok_to_spindle(); }
// bool LaserTool::busy() {
//     // return true when not paused, on, and ramping up to speed
//     if (paused || (direction == SPINDLE_OFF)) {
//         return false;
//     }
//     return true;
// }

float LaserTool::get_speed() { return speed; }

spDirection LaserTool::get_direction() { return direction; }

void LaserTool::stop() {
    paused = false;
    speed = 0;
    direction = SPINDLE_OFF;

    this->complete_change();
}

// called from a command that was queued when the default set_speed and set_direction returned true
// ALSO called from the loader right before a move
// we are handed the gcode model to use
void LaserTool::engage(const GCodeState_t &gm) {
    if ((direction == gm.spindle_direction) && fp_EQ(speed, gm.spindle_speed)) {
        // nothing changed
        return;
    }

    // // special handling for reversals - we set the speed to zero and ramp up
    // if ((gm.spindle_direction != direction) && (direction != SPINDLE_OFF) && (gm.spindle_direction != SPINDLE_OFF)) {
    //     speed_actual = 0;
    // }

    speed = gm.spindle_speed;
    direction = gm.spindle_direction;

    // handle the rest
    this->complete_change();
}

bool LaserTool::is_on() { return (direction != SPINDLE_OFF); }

// LaserTool-specific functions
void LaserTool::set_pwm_output(const uint8_t pwm_pin_number) {
    if (pwm_pin_number == 0) {
        pwm_output = nullptr;
    } else {
        pwm_output = d_out[pwm_pin_number - 1];
        pwm_output->setEnabled(IO_ENABLED);
        // set the frequency on the output -- not here
        // set the polarity on the output -- not here
    }
}
void LaserTool::set_enable_output(const uint8_t enable_pin_number) {
    if (enable_pin_number == 0) {
        enable_output = nullptr;
    } else {
        enable_output = d_out[enable_pin_number - 1];
        enable_output->setEnabled(IO_ENABLED);
        // set the polarity on the output -- not here
    }
}

void LaserTool::set_frequency(float new_frequency)
{
    if (pwm_output) {
        pwm_output->setFrequency(new_frequency);
    }
}
float LaserTool::get_frequency()
{
    if (pwm_output) {
        return pwm_output->getFrequency();
    }
    return 0.0;
}

// Private functions

void LaserTool::complete_change() {
    // if the spindle is not on (or paused), make sure we stop it
    if (paused || direction == SPINDLE_OFF) {
        if (enable_output != nullptr) {
            enable_output->setValue(false);
        }

        return;
    } else if (direction == SPINDLE_CW) {
        if (enable_output != nullptr) {
            enable_output->setValue(true);
        }
    } else if (direction == SPINDLE_CCW) {
        if (enable_output != nullptr) {
            enable_output->setValue(true);
        }
    }
}

#endif  // End of include guard: LASER_TOOLHEAD_H_ONCE
