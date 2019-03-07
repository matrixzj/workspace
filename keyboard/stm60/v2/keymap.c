#include "stm60.h"

#define _______ KC_TRNS

#define _BASE 0
#define _FN1  1
#define ESCTL CTL_T(KC_ESC)

const uint8_t RGBLED_BREATHING_INTERVALS[] PROGMEM = {120, 80, 40, 20};

enum custom_keycodes {
    MY_USER = SAFE_RANGE,
    MY_PASS1,
    MY_PASS2,
    MY_PIPE,
    FW_ILO,
    FW_DEV,
    FW_PRD,
    LEDOFF
};

uint32_t key_timer;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch(keycode) {
            case MY_USER:
                SEND_STRING("aaa");
                return false;
            case MY_PASS1:
                SEND_STRING("bbb");
                return false;
            case MY_PASS2:
                SEND_STRING("ccc");
                return false;
            case MY_PIPE:
                SEND_STRING("|");
                return false;
            case FW_PRD:
                SEND_STRING("ddd");
                return false;
            case FW_DEV:
                SEND_STRING("xxx");
                return false;
            case FW_ILO:
                SEND_STRING("fff");
                return false;
            case LEDOFF:
                rgblight_toggle();
                return false;
            default:
                switch (timer_elapsed32(key_timer) % 3) {
                    case 0:
                        rgblight_increase_hue();
                        rgblight_mode(4);
                        return true;
                    case 1:
                        rgblight_increase_sat();
                        rgblight_mode(4);
                        return true;
                    case 2:
                        rgblight_increase_val();
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

const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Layer 0: Default Layer
     * ,-----------------------------------------------------------.
     * | ` |  1|  2|  3|  4|  5|  6|  7|  8|  9|  0|  -|  =| \|DEL |
     * |-----------------------------------------------------------|
     * |Tab  |  Q|  W|  E|  R|  T|  Y|  U|  I|  O|  P|  [|  ]|  Bsp|
     * |-----------------------------------------------------------|
     * |Contro|  A|  S|  D|  F|  G|  H|  J|  K|  L|  ;|  '|Enter   |
     * |-----------------------------------------------------------|
     * |Shift   |  Z|  X|  C|  V|  B|  N|  M|  ,|  .|  /|  Shift   |
     * |-----------------------------------------------------------'
     * |     |Fn |Gui  |         Space             |Alt  |Fn |     |
     * `-----------------------------------------------------------'
     */

    [_BASE] = KEYMAP_wkl(
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSLS,  RGB_TOG,    \
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSPC,    		\
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_ENT,     			\
        KC_LSFT,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,  RGB_M_T,                        \
        KC_NO,    KC_LALT,  KC_LGUI,                                KC_SPC,                                 KC_RALT,  KC_RGUI,  KC_NO     ),

  // 1: Function Layer
    [_FN1] = KEYMAP_wkl(
        RESET,    KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   _______,   _______,	\
        MY_PIPE,  KC_VOLD,  KC_VOLU,  KC_MUTE,  _______,  _______,  _______,  _______,  _______,  _______,  FW_ILO,   FW_DEV,   FW_PRD,   _______,		\
        _______,  KC_MRWD,  KC_MPLY,  KC_MFFD,  _______,  _______,  KC_LEFT,  KC_DOWN,  KC_UP,    KC_RGHT,  MY_USER,  MY_PASS1, _______,			\
        _______,  RGB_TOG,  RGB_M_T,  RGB_HUI,  RGB_SAI,  RGB_VAI,  _______,  _______,  _______,  _______,  MY_PASS2, _______,  _______,                     	\
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______	  ),
};
