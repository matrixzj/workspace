// M60-A layout
#include QMK_KEYBOARD_H

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
                SEND_STRING("aaa"); // username!
                return false;
            case C_USER:
                SEND_STRING("bbb"); // passwd1!
                return false;
            case C_PASS2:
                SEND_STRING("ccc"); // another passwd!
                return false;
            case C_PIPE:
                SEND_STRING("|"); // this is our macro!
                return false;
            default:
                return true;
        }
    }
    return true;
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

// Default layer
[0] = LAYOUT_60_hhkb(
    KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS,  KC_EQL,  KC_BSLS,  KC_DEL,    
    KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC,  KC_RBRC, KC_BSPC,   
    ESCTL,   KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,  KC_ENT,
    KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,  MO(1),     
             MO(1),   KC_LGUI,                            KC_SPC,                             KC_RALT, MO(1)),

// Fn1 Layer
[1] = LAYOUT_60_hhkb(
    RESET,   KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,   KC_F12,  KC_INS,   KC_TRNS,
    C_PIPE,  KC_VOLD, KC_VOLU, KC_MUTE, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS,
    KC_TRNS, KC_MRWD, KC_MPLY, KC_MFFD, KC_TRNS, KC_TRNS, KC_LEFT, KC_DOWN, KC_UP,   KC_RIGHT,C_USER,  C_PASS1,  KC_ENT,
    KC_LSFT, BR_OFF,  EF_INC,  ES_INC,  BR_INC,  H1_INC,  S1_INC,  H2_INC,  S2_INC,  KC_TRNS, C_PASS2, KC_TRNS,  KC_TRNS,
             KC_TRNS, KC_TRNS,                            KC_TRNS,                            KC_TRNS, KC_TRNS),
};
