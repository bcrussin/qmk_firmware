/* Copyright 2023 Colin Lam (Ploopy Corporation)
 * Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <math.h>
#include "process_tap_dance.h"
#include QMK_KEYBOARD_H

#define TAPPING_TERM 175

// #define MIDDLE_MOUSE_LT LT(0, KC_BTN3)
#define MAC_OPEN_FLOW LGUI(LALT(KC_SPACE))
#define MAC_MOUSE_JUMP LGUI(LALT(KC_D))

enum LAYERS {
    _DEFAULT,
    _SYSTEM,
    _NAVIGATION,
};

enum CUSTOM_KEYCODES {
    MIDDLE_DRAG = SAFE_RANGE
};

static bool is_scroll_mode = false;

static float scroll_accum_x = 0.0f;
static float scroll_accum_y = 0.0f;

// void scroll_click_press(tap_dance_state_t *state, void *user_data) {
//     if (state->count == 1) {
//         register_code(KC_BTN3);  // press middle mouse immediately
//     }
// }

// void scroll_click_finished(tap_dance_state_t *state, void *user_data) {
//     if (state->count == 1 && state->pressed) {
//         // Hold detected → switch to scroll mode
//         unregister_code(KC_BTN3);
//         is_scroll_mode = true;
//     }
// }

// void scroll_click_reset(tap_dance_state_t *state, void *user_data) {
//     unregister_code(KC_BTN3);
//     is_scroll_mode = false;
// }

// enum TAPDANCES {
//     TD_SCROLL_CLICK
// };

// tap_dance_action_t tap_dance_actions[] = {
//     [TD_SCROLL_CLICK] = ACTION_TAP_DANCE_FN_ADVANCED(scroll_click_press, scroll_click_finished, scroll_click_reset),
// };

// enum MACROS {
//     MAC_OPEN_FLOW = SAFE_RANGE
// }

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_DEFAULT] = LAYOUT( MO(_SYSTEM), MO(_NAVIGATION), MIDDLE_DRAG, KC_BTN2, KC_BTN1, KC_BTN3 ),
    [_SYSTEM] = LAYOUT( _______, KC_F13, KC_F14, MAC_MOUSE_JUMP, MAC_OPEN_FLOW, DPI_CONFIG ),
    [_NAVIGATION] = LAYOUT(KC_WWW_BACK, _______, KC_WWW_FORWARD, LCTL(LSFT(KC_TAB)), LCTL(KC_W), LCTL(KC_TAB))
};

// static bool is_scroll_mode = false;
static bool key_held = false;
static uint16_t drag_timer = 0;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case MIDDLE_DRAG:
            if (record->event.pressed) {
                // If double tap, hold middle mouse.
                // Otherwise, enable drag scroll.
                if (timer_elapsed(drag_timer) < TAPPING_TERM) {
                    register_code(KC_BTN3); // tap → middle click
                } else {
                    is_scroll_mode = true;
                }

                drag_timer = timer_read();
                key_held = true;
            } else {
                is_scroll_mode = false;

                if (key_held) {
                    if (timer_elapsed(drag_timer) < TAPPING_TERM) {
                        tap_code(KC_BTN3); // Single tap = middle click
                    } else if (is_scroll_mode) {
                        is_scroll_mode = false; // Stop drag scrolling if currently enabled
                    } else {
                        unregister_code(KC_BTN3); // Release middle click if it is held
                    }
                }

                key_held = false;
                drag_timer = timer_read();
            }
            return false; // prevent default handling
    }
    return true;
}


report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    // --- MOUSE ACCELERATION ALGORITHM (QUADRATIC) ---
    // Calculate 2D velocity magnitude (speed)
    // https://github.com/gorlix/my-adept
    float speed = sqrt(pow(mouse_report.x, 2) + pow(mouse_report.y, 2));

    if (speed > ACCEL_OFFSET) {
        // Quadratic acceleration formula
        // Use a smaller divisor (0.001) since we are squaring the speed difference
        float factor = 1.0f + pow(speed - ACCEL_OFFSET, 2) * 0.001f * ACCEL_SLOPE;

        // Cap the acceleration factor
        if (factor > ACCEL_LIMIT) {
            factor = ACCEL_LIMIT;
        }

        // Apply scaling
        mouse_report.x = (int16_t)(mouse_report.x * factor);
        mouse_report.y = (int16_t)(mouse_report.y * factor);
    }

    if (is_scroll_mode) {
        // Currently locked to vertical scroll only

        // mouse_report.h = (float)mouse_report.x * SCROLL_SENSITIVITY;
        mouse_report.v = (float)mouse_report.y * SCROLL_SENSITIVITY * -1;

        // DISABLED - Disable scroll axis based on curr direction (buggy):

        // float h_ratio = (float)abs(mouse_report.h) / abs(mouse_report.v);
        // if (h_ratio > 5 && mouse_report.h > 8) {
        //     mouse_report.v = 0;
        // } else {
        //     mouse_report.h = 0;
        // }

        scroll_accum_x = 0;
        scroll_accum_y = 0;

        // DISABLED - High-res scroll:

        // Accumulate input (already accelerated) * sensitivity
        // scroll_accum_x += (float)mouse_report.x * SCROLL_SENSITIVITY;
        // scroll_accum_y += (float)mouse_report.y * SCROLL_SENSITIVITY;

        // // Calculate Integer part from Accumulator
        // int8_t v_part = (int8_t)(scroll_accum_y * -1.0f); // Invert Y for Scroll V
        // int8_t h_part = (int8_t)(scroll_accum_x);

        // // Assign directly to report (Allow both V and H)
        // mouse_report.v = v_part;
        // mouse_report.h = h_part;

        // // Update accumulators by subtracting consumed integer parts
        // if (mouse_report.v != 0) {
        //     scroll_accum_y -= (float)(mouse_report.v * -1);
        // }
        // if (mouse_report.h != 0) {
        //     scroll_accum_x -= (float)(mouse_report.h);
        // }

        // Lock cursor movement
        mouse_report.x = 0;
        mouse_report.y = 0;
    }

    return mouse_report;
}


// bool process_record_user(uint16_t keycode, keyrecord_t* record) {
//     switch (keycode) {
//         case MIDDLE_DRAG:
//             if (record->event.pressed) {
//                 tap_code(KC_BTN3);
//             } else {
//                 is_scroll_mode = true;
//             }
//             return false;
//         }
//     return true;
// }

// void matrix_scan_user(void) {
//     if (key_held && !drag_active && timer_elapsed(drag_timer) >= TAPPING_TERM) {
//         tap_code16(DRAG_SCROLL); // enter drag-scroll mode
//         drag_active = true;
//     }
// }
