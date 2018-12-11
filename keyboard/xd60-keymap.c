#include QMK_KEYBOARD_H
#include "action_layer.h"

#define _BASE 0
#define _FN1  1 
#define ESCTL CTL_T(KC_ESC)

const uint8_t RGBLED_BREATHING_INTERVALS[] PROGMEM = {240, 160, 80, 40};

enum custom_keycodes {
    C_PASS1 = SAFE_RANGE,
    C_USER,
    C_PASS2,
    C_PIPE,
    LEDOFF
};

uint32_t key_timer;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch(keycode) {
            case C_PASS1:
                SEND_STRING("a"); // username!
                return false;
            case C_USER:
                SEND_STRING("b"); // passwd1!
                return false;
            case C_PASS2:
                SEND_STRING("c"); // this is our macro!
                return false;
            case C_PIPE:
                SEND_STRING("|"); // this is our macro!
                return false;
     	    case LEDOFF:
            	rgblight_toggle();
	        	return false;
            default:
                switch (timer_elapsed32(key_timer) % 7) {
                  case 0:
                    rgblight_increase_hue();
                    rgblight_mode(4);
                    return true;
                  case 1:
                    rgblight_decrease_hue();
                    rgblight_mode(4);
                    return true;
                  case 2:
                    rgblight_increase_sat();
                    rgblight_mode(4);
                    return true;
                  case 3:
                    rgblight_decrease_sat();
                    rgblight_mode(4);
                    return true;
                  case 4:
                    rgblight_increase_val();
                    rgblight_mode(4);
                    return true;
                  case 5:
                    rgblight_decrease_val();
                    rgblight_mode(4);
                    return true;
                  case 6:
                    rgblight_step();
                    rgblight_mode(4);
                    return true;
                  default:
                    rgblight_increase_hue();
                    rgblight_mode(4);
                    return true;
                }
        }
    }
    return true;
};

enum combo_events {
  SYM_PIPE,
};

const uint16_t PROGMEM sym_pipe[] = {KC_RSHIFT, KC_TAB, COMBO_END};

combo_t key_combos[COMBO_COUNT] = {
  [SYM_PIPE] = COMBO_ACTION(sym_pipe),
};

void process_combo_event(uint8_t combo_index, bool pressed) {
  switch(combo_index) {
    case SYM_PIPE:
      if (pressed) {
        register_code(KC_RSHIFT);
        register_code(KC_BSLS);
        unregister_code(KC_BSLS);
        unregister_code(KC_RSHIFT);
      }
      break;
  }
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

  // 0: Base Layer
  [_BASE] = LAYOUT_all(
      KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,   KC_5,   KC_6,   KC_7,   KC_8,   KC_9,    KC_0,    KC_MINS,  KC_EQL,  KC_BSLS,  KC_ESC,    \
      KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,   KC_T,   KC_Y,   KC_U,   KC_I,   KC_O,    KC_P,    KC_LBRC,  KC_RBRC,           KC_BSPC,   \
      ESCTL,   KC_A,    KC_S,    KC_D,    KC_F,   KC_G,   KC_H,   KC_J,   KC_K,   KC_L,    KC_SCLN, KC_QUOT,  KC_NO,             KC_ENT,    \
      KC_LSFT, KC_NO,   KC_Z,    KC_X,    KC_C,   KC_V,   KC_B,   KC_N,   KC_M,   KC_COMM, KC_DOT,  KC_SLSH,  KC_NO,   KC_RSFT,  MO(1),     \
      KC_NO,   MO(1),   KC_LGUI,                          KC_SPC,                          KC_NO,   MO(1),    KC_NO,   KC_RALT,  KC_NO),

  // 1: Function Layer
  [_FN1] = LAYOUT_all(
      RESET,   KC_F1,   KC_F2,   KC_F3,   KC_F4,  KC_F5,  KC_F6,  KC_F7,  KC_F8,  KC_F9,   KC_F10,  KC_F11,   KC_F12,  KC_DEL,   KC_NO,     \
      C_PIPE,  KC_VOLD, KC_VOLU, KC_MUTE, KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS,KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS,           KC_TRNS,   \
      KC_TRNS, KC_MRWD, KC_MPLY, KC_MFFD, KC_TRNS,KC_TRNS,KC_LEFT,KC_DOWN,KC_UP,  KC_RIGHT,C_USER,  C_PASS1,  KC_NO,             KC_ENT,    \
      KC_LSFT, LEDOFF,  F(2),    F(3),    F(4),   F(5),   F(6),   F(7),   F(8),   KC_TRNS, KC_TRNS, C_PASS2,  KC_TRNS, KC_TRNS,  KC_TRNS,   \
      KC_NO,   KC_TRNS, KC_TRNS,                          KC_TRNS,                         KC_NO,   KC_TRNS,  KC_NO,   KC_TRNS,  KC_NO),

};

enum function_id {
    RGBLED_TOGGLE,
    RGBLED_STEP_MODE,
    RGBLED_INCREASE_HUE,
    RGBLED_DECREASE_HUE,
    RGBLED_INCREASE_SAT,
    RGBLED_DECREASE_SAT,
    RGBLED_INCREASE_VAL,
    RGBLED_DECREASE_VAL,
};

const uint16_t PROGMEM fn_actions[] = {
#ifdef RGBLIGHT_ENABLE
  [1]  = ACTION_FUNCTION(RGBLED_TOGGLE),
  [2]  = ACTION_FUNCTION(RGBLED_STEP_MODE),
  [3]  = ACTION_FUNCTION(RGBLED_INCREASE_HUE),
  [4]  = ACTION_FUNCTION(RGBLED_DECREASE_HUE),
  [5]  = ACTION_FUNCTION(RGBLED_INCREASE_SAT),
  [6]  = ACTION_FUNCTION(RGBLED_DECREASE_SAT),
  [7]  = ACTION_FUNCTION(RGBLED_INCREASE_VAL),
  [8]  = ACTION_FUNCTION(RGBLED_DECREASE_VAL),
#else
  [1]  = ACTION_NO,
  [2]  = ACTION_NO,
  [3]  = ACTION_NO,
  [4]  = ACTION_NO,
  [5]  = ACTION_NO,
  [6]  = ACTION_NO,
  [7]  = ACTION_NO,
  [8]  = ACTION_NO,
#endif
};

void action_function(keyrecord_t *record, uint8_t id, uint8_t opt) {
#ifdef RGBLIGHT_ENABLE
  switch (id) {
    case RGBLED_TOGGLE:
      //led operations
      if (record->event.pressed) {
        rgblight_toggle();
      }

      break;
    case RGBLED_INCREASE_HUE:
      if (record->event.pressed) {
        rgblight_increase_hue();
      }
      break;
    case RGBLED_DECREASE_HUE:
      if (record->event.pressed) {
        rgblight_decrease_hue();
      }
      break;
    case RGBLED_INCREASE_SAT:
      if (record->event.pressed) {
        rgblight_increase_sat();
      }
      break;
    case RGBLED_DECREASE_SAT:
      if (record->event.pressed) {
        rgblight_decrease_sat();
      }
      break;
      case RGBLED_INCREASE_VAL:
        if (record->event.pressed) {
          rgblight_increase_val();
        }
        break;
      case RGBLED_DECREASE_VAL:
        if (record->event.pressed) {
          rgblight_decrease_val();
        }
        break;
      case RGBLED_STEP_MODE:
        if (record->event.pressed) {
          rgblight_step();
        }
        break;
#endif
  }
}
