/* Copyright 2017 Jason Williams (Wilba)
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
#if RGB_BACKLIGHT_ENABLED

#if defined (RGB_BACKLIGHT_ZEAL60) || defined (RGB_BACKLIGHT_ZEAL65) || defined (RGB_BACKLIGHT_M60_A) || defined(RGB_BACKLIGHT_M6_B) || defined(RGB_BACKLIGHT_KOYU) || defined(RGB_BACKLIGHT_HS60)
#else
#error None of the following was defined: RGB_BACKLIGHT_ZEAL60, RGB_BACKLIGHT_ZEAL65, RGB_BACKLIGHT_M60_A, RGB_BACKLIGHT_M6_B, RGB_BACKLIGHT_KOYU
#endif

#ifndef MAX
    #define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

#ifndef MIN
    #define MIN(a,b) ((a) < (b)? (a): (b))
#endif

#include "quantum.h"
#include "rgb_backlight.h"
#include "rgb_backlight_api.h"
#include "rgb_backlight_keycodes.h"

#if !defined(RGB_BACKLIGHT_HS60)
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "drivers/avr/i2c_master.h"
#else
#include "ch.h"
#include "hal.h"
#include "drivers/arm/i2c_master.h"
#include "tmk_core/common/eeprom.h"
#endif
#include "progmem.h"
#include "quantum/color.h"

#if defined (RGB_BACKLIGHT_M6_B)
#include "drivers/issi/is31fl3218.h"
#define BACKLIGHT_LED_COUNT 6
#elif defined (RGB_BACKLIGHT_HS60)
#include "drivers/issi/is31fl3733.h"
#define BACKLIGHT_LED_COUNT 64
#else
#include "drivers/issi/is31fl3731.h"
#define BACKLIGHT_LED_COUNT 72
#endif

#define BACKLIGHT_EFFECT_MAX 10

backlight_config g_config = {
	.use_split_backspace = RGB_BACKLIGHT_USE_SPLIT_BACKSPACE,
	.use_split_left_shift = RGB_BACKLIGHT_USE_SPLIT_LEFT_SHIFT,
	.use_split_right_shift = RGB_BACKLIGHT_USE_SPLIT_RIGHT_SHIFT,
	.use_7u_spacebar = RGB_BACKLIGHT_USE_7U_SPACEBAR,
	.use_iso_enter = RGB_BACKLIGHT_USE_ISO_ENTER,
	.disable_hhkb_blocker_leds = RGB_BACKLIGHT_DISABLE_HHKB_BLOCKER_LEDS,
	.disable_when_usb_suspended = RGB_BACKLIGHT_DISABLE_WHEN_USB_SUSPENDED,
	.disable_after_timeout = RGB_BACKLIGHT_DISABLE_AFTER_TIMEOUT,
	.brightness = 255,
	.effect = RGB_BACKLIGHT_EFFECT,
	.effect_speed = 0,
	.color_1 = { .h = 0, .s = 255 },
	.color_2 = { .h = 127, .s = 255 },
	.caps_lock_indicator = { .color = { .h = 0, .s = 0 }, .index = 255 },
	.layer_1_indicator = { .color = { .h = 0, .s = 0 }, .index = 255 },
	.layer_2_indicator = { .color = { .h = 0, .s = 0 }, .index = 255 },
	.layer_3_indicator = { .color = { .h = 0, .s = 0 }, .index = 255 },
	.alphas_mods = {
		RGB_BACKLIGHT_ALPHAS_MODS_ROW_0,
		RGB_BACKLIGHT_ALPHAS_MODS_ROW_1,
		RGB_BACKLIGHT_ALPHAS_MODS_ROW_2,
		RGB_BACKLIGHT_ALPHAS_MODS_ROW_3,
		RGB_BACKLIGHT_ALPHAS_MODS_ROW_4 },
#if defined(RGB_BACKLIGHT_M6_B)
	.custom_color = { { 0, 255 }, { 43, 255 }, { 85, 255 }, { 128, 255 }, { 171, 255 }, { 213, 255 } }
#endif
};

bool g_suspend_state = false;
uint8_t g_indicator_state = 0;

// Global tick at 20 Hz
uint32_t g_tick = 0;

// Ticks since this key was last hit.
uint8_t g_key_hit[BACKLIGHT_LED_COUNT];

// Ticks since any key was last hit.
uint32_t g_any_key_hit = 0;

#if defined(RGB_BACKLIGHT_HS60)

// This is a 7-bit address, that gets left-shifted and bit 0
// set to 0 for write, 1 for read (as per I2C protocol)
// ADDR_2 is not needed. it is here as a dummy
#define ISSI_ADDR_1 0x50
#define ISSI_ADDR_2 0x50

const is31_led g_is31_leds[DRIVER_LED_TOTAL] = {
/* Refer to IS31 manual for these locations
 *   driver
 *   |  R location
 *   |  |      G location
 *   |  |      |      B location
 *   |  |      |      | */
    {0, B_1,   A_1,   C_1}, //LA1
    {0, E_1,   D_1,   F_1}, //LA2
    {0, H_1,   G_1,   I_1}, //LA3
    {0, K_1,   J_1,   L_1}, //LA4
    {0, B_2,   A_2,   C_2}, //LA5
    {0, E_2,   D_2,   F_2}, //LA6
    {0, H_2,   G_2,   I_2}, //LA7
    {0, K_2,   J_2,   L_2}, //LA8
    {0, B_3,   A_3,   C_3}, //LA9
    {0, E_3,   D_3,   F_3}, //LA10
    {0, H_3,   G_3,   I_3}, //LA11
    {0, K_3,   J_3,   L_3}, //LA12
    {0, B_4,   A_4,   C_4}, //LA13
    {0, E_4,   D_4,   F_4}, //LA14
    {0, H_4,   G_4,   I_4}, //LA15
    {0, K_4,   J_4,   L_4}, //LA16
    {0, B_5,   A_5,   C_5}, //LA17
    {0, E_5,   D_5,   F_5}, //LA18
    {0, H_5,   G_5,   I_5}, //LA19
    {0, K_5,   J_5,   L_5}, //LA20
    {0, B_6,   A_6,   C_6}, //LA21
    {0, E_6,   D_6,   F_6}, //LA22
    {0, H_6,   G_6,   I_6}, //LA23
    {0, K_6,   J_6,   L_6}, //LA24
    {0, B_7,   A_7,   C_7}, //LA25
    {0, E_7,   D_7,   F_7}, //LA26
    {0, H_7,   G_7,   I_7}, //LA27
    {0, K_7,   J_7,   L_7}, //LA28
    {0, B_8,   A_8,   C_8}, //LA29
    {0, E_8,   D_8,   F_8}, //LA30
    {0, H_8,   G_8,   I_8}, //LA31
    {0, K_8,   J_8,   L_8}, //LA32
    {0, B_9,   A_9,   C_9}, //LA33
    {0, E_9,   D_9,   F_9}, //LA34
    {0, H_9,   G_9,   I_9}, //LA35
    {0, K_9,   J_9,   L_9}, //LA36
    {0, B_10,  A_10,  C_10}, //LA37
    {0, E_10,  D_10,  F_10}, //LA38
    {0, H_10,  G_10,  I_10}, //LA39
    {0, K_10,  J_10,  L_10}, //LA40
    {0, B_11,  A_11,  C_11}, //LA41
    {0, E_11,  D_11,  F_11}, //LA42
    {0, H_11,  G_11,  I_11}, //LA43
    {0, K_11,  J_11,  L_11}, //LA44
    {0, B_12,  A_12,  C_12}, //LA45
    {0, E_12,  D_12,  F_12}, //LA46
    {0, H_12,  G_12,  I_12}, //LA47
    {0, K_12,  J_12,  L_12}, //LA48
    {0, B_13,  A_13,  C_13}, //LA49
    {0, E_13,  D_13,  F_13}, //LA50
    {0, H_13,  G_13,  I_13}, //LA51
    {0, K_13,  J_13,  L_13}, //LA52
    {0, B_14,  A_14,  C_14}, //LA53
    {0, E_14,  D_14,  F_14}, //LA54
    {0, H_14,  G_14,  I_14}, //LA55
    {0, K_14,  J_14,  L_14}, //LA56
    {0, B_15,  A_15,  C_15}, //LA57
    {0, E_15,  D_15,  F_15}, //LA58
    {0, H_15,  G_15,  I_15}, //LA59
    {0, K_15,  J_15,  L_15}, //LA60
    {0, B_16,  A_16,  C_16}, //LA61
    {0, E_16,  D_16,  F_16}, //LA62
    {0, H_16,  G_16,  I_16}, //LA63
    {0, K_16,  J_16,  L_16}, //LA64
};

#elif !defined(RGB_BACKLIGHT_M6_B)
// This is a 7-bit address, that gets left-shifted and bit 0
// set to 0 for write, 1 for read (as per I2C protocol)
#define ISSI_ADDR_1 0x74
#define ISSI_ADDR_2 0x76

const is31_led g_is31_leds[DRIVER_LED_TOTAL] = {
/* Refer to IS31 manual for these locations
 *   driver
 *   |  R location
 *   |  |      G location
 *   |  |      |      B location
 *   |  |      |      | */
	{0, C2_1,  C3_1,  C4_1},  // LA0
	{0, C1_1,  C3_2, C4_2},   // LA1
	{0, C1_2,  C2_2, C4_3},   // LA2
	{0, C1_3,  C2_3, C3_3},   // LA3
	{0, C1_4,  C2_4, C3_4},   // LA4
	{0, C1_5,  C2_5, C3_5},   // LA5
	{0, C1_6,  C2_6, C3_6},   // LA6
	{0, C1_7,  C2_7, C3_7},   // LA7
	{0, C1_8,  C2_8, C3_8},   // LA8
	{0, C9_1,  C8_1,  C7_1},  // LA9
	{0, C9_2,  C8_2, C7_2},   // LA10
	{0, C9_3,  C8_3, C7_3},   // LA11
	{0, C9_4,  C8_4, C7_4},   // LA12
	{0, C9_5,  C8_5, C7_5},   // LA13
	{0, C9_6,  C8_6, C7_6},   // LA14
	{0, C9_7,  C8_7, C6_6},   // LA15
	{0, C9_8,  C7_7, C6_7},   // LA16
	{0, C8_8,  C7_8, C6_8},   // LA17

	{0, C2_9,  C3_9,  C4_9},  // LB0
	{0, C1_9,  C3_10, C4_10}, // LB1
	{0, C1_10, C2_10, C4_11}, // LB2
	{0, C1_11, C2_11, C3_11}, // LB3
	{0, C1_12, C2_12, C3_12}, // LB4
	{0, C1_13, C2_13, C3_13}, // LB5
	{0, C1_14, C2_14, C3_14}, // LB6
	{0, C1_15, C2_15, C3_15}, // LB7
	{0, C1_16, C2_16, C3_16}, // LB8
	{0, C9_9,  C8_9,  C7_9},  // LB9
	{0, C9_10, C8_10, C7_10}, // LB10
	{0, C9_11, C8_11, C7_11}, // LB11
	{0, C9_12, C8_12, C7_12}, // LB12
	{0, C9_13, C8_13, C7_13}, // LB13
	{0, C9_14, C8_14, C7_14}, // LB14
	{0, C9_15, C8_15, C6_14}, // LB15
	{0, C9_16, C7_15, C6_15}, // LB16
	{0, C8_16, C7_16, C6_16}, // LB17

	{1, C2_1,  C3_1,  C4_1},  // LC0
	{1, C1_1,  C3_2, C4_2},   // LC1
	{1, C1_2,  C2_2, C4_3},   // LC2
	{1, C1_3,  C2_3, C3_3},   // LC3
	{1, C1_4,  C2_4, C3_4},   // LC4
	{1, C1_5,  C2_5, C3_5},   // LC5
	{1, C1_6,  C2_6, C3_6},   // LC6
	{1, C1_7,  C2_7, C3_7},   // LC7
	{1, C1_8,  C2_8, C3_8},   // LC8
	{1, C9_1,  C8_1,  C7_1},  // LC9
	{1, C9_2,  C8_2, C7_2},   // LC10
	{1, C9_3,  C8_3, C7_3},   // LC11
	{1, C9_4,  C8_4, C7_4},   // LC12
	{1, C9_5,  C8_5, C7_5},   // LC13
	{1, C9_6,  C8_6, C7_6},   // LC14
	{1, C9_7,  C8_7, C6_6},   // LC15
	{1, C9_8,  C7_7, C6_7},   // LC16
	{1, C8_8,  C7_8, C6_8},   // LC17

	{1, C2_9,  C3_9,  C4_9},  // LD0
	{1, C1_9,  C3_10, C4_10}, // LD1
	{1, C1_10, C2_10, C4_11}, // LD2
	{1, C1_11, C2_11, C3_11}, // LD3
	{1, C1_12, C2_12, C3_12}, // LD4
	{1, C1_13, C2_13, C3_13}, // LD5
	{1, C1_14, C2_14, C3_14}, // LD6
	{1, C1_15, C2_15, C3_15}, // LD7
	{1, C1_16, C2_16, C3_16}, // LD8
	{1, C9_9,  C8_9,  C7_9},  // LD9
	{1, C9_10, C8_10, C7_10}, // LD10
	{1, C9_11, C8_11, C7_11}, // LD11
	{1, C9_12, C8_12, C7_12}, // LD12
	{1, C9_13, C8_13, C7_13}, // LD13
	{1, C9_14, C8_14, C7_14}, // LD14
	{1, C9_15, C8_15, C6_14}, // LD15
	{1, C9_16, C7_15, C6_15}, // LD16
	{1, C8_16, C7_16, C6_16}, // LD17
};
#endif // !defined(RGB_BACKLIGHT_M6_B)


typedef struct Point {
	uint8_t x;
	uint8_t y;
} Point;


// index in range 0..71 (LA0..LA17, LB0..LB17, LC0..LC17, LD0..LD17)
// point values in range x=0..224 y=0..64
// origin is center of top-left key (i.e Esc)
#if defined (RGB_BACKLIGHT_ZEAL65)
const Point g_map_led_to_point[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA0..LA17
	{120,16}, {104,16}, {88,16}, {72,16}, {56,16}, {40,16}, {24,16}, {4,16}, {4,32},
	{128,0}, {112,0}, {96,0}, {80,0}, {64,0}, {48,0}, {32,0}, {16,0}, {0,0},
	// LB0..LB17
	{144,0}, {160,0}, {176,0}, {192,0}, {216,0}, {224,0}, {240,0}, {240,16}, {240,32},
	{136,16}, {152,16}, {168,16}, {184,16}, {200,16}, {220,16}, {240,48}, {240,64}, {224,64},
	// LC0..LC17
	{96,64}, {100,48}, {84,48}, {68,48}, {52,48}, {36,48}, {255,255}, {48,60}, {28,64},
	{108,32}, {92,32}, {76,32}, {60,32}, {44,32}, {28,32}, {20,44}, {10,48}, {4,64},
	// LD0..LD17
	{124,32}, {140,32}, {156,32}, {172,32}, {188,32}, {214,32}, {180,48}, {202,48}, {224,48},
	{116,48}, {132,48}, {148,48}, {164,48}, {255,255}, {144,60}, {164,64}, {188,64}, {208,64}
};
const Point g_map_led_to_point_polar[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA0..LA17
	{64,128}, {75,132}, {84,145}, {91,164}, {97,187}, {102,213}, {105,242}, {109,255}, {128,243},
	{61,255}, {67,255}, {72,255}, {77,255}, {82,255}, {86,255}, {90,255}, {93,255}, {96,255},
	// LB0..LB17
	{56,255}, {51,255}, {46,255}, {42,255}, {37,255}, {35,255}, {32,255}, {19,255}, {0,255},
	{53,132}, {44,145}, {37,164}, {31,187}, {26,213}, {22,249}, {237,255}, {224,255}, {221,255},
	// LC0..LC17
	{184,255}, {179,135}, {170,149}, {163,169}, {157,193}, {153,220}, {255,255}, {167,255}, {165,255},
	{128,26}, {128,60}, {128,94}, {128,128}, {128,162}, {128,196}, {145,233}, {148,255}, {161,255},
	// LD0..LD17
	{0,9}, {0,43}, {0,77}, {0,111}, {0,145}, {0,201}, {224,181}, {230,217}, {235,255},
	{189,128}, {200,131}, {210,141}, {218,159}, {255,255}, {201,228}, {206,255}, {213,255}, {218,255}
};
#elif defined (RGB_BACKLIGHT_KOYU)
const Point g_map_led_to_point[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA0..LA17
	{120,16}, {104,16}, {88,16}, {72,16}, {56,16}, {40,16}, {24,16}, {4,16}, {4,32},
	{128,0}, {112,0}, {96,0}, {80,0}, {64,0}, {48,0}, {32,0}, {16,0}, {0,0},
	// LB0..LB17
	{144,0}, {160,0}, {176,0}, {192,0}, {208,0}, {224,0}, {240,0}, {240,16}, {240,32},
	{136,16}, {152,16}, {168,16}, {184,16}, {200,16}, {220,16}, {240,48}, {240,64}, {224,64},
	// LC0..LC17
	{112,64}, {100,48}, {84,48}, {68,48}, {52,48}, {36,48}, {64,60}, {44,60}, {24,64},
	{108,32}, {92,32}, {76,32}, {60,32}, {44,32}, {28,32}, {255,255}, {10,48}, {4,64},
	// LD0..LD17
	{124,32}, {140,32}, {156,32}, {172,32}, {188,32}, {214,32}, {180,48}, {202,48}, {224,48},
	{116,48}, {132,48}, {148,48}, {164,48}, {255,255}, {160,60}, {180,64}, {208,64}, {255,255}
};
const Point g_map_led_to_point_polar[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA0..LA17
	{64,128}, {75,132}, {84,145}, {91,164}, {97,187}, {102,213}, {105,242}, {109,255}, {128,243},
	{61,255}, {67,255}, {72,255}, {77,255}, {82,255}, {86,255}, {90,255}, {93,255}, {96,255},
	// LB0..LB17
	{56,255}, {51,255}, {46,255}, {42,255}, {38,255}, {35,255}, {32,255}, {19,255}, {0,255},
	{53,132}, {44,145}, {37,164}, {31,187}, {26,213}, {22,249}, {237,255}, {224,255}, {221,255},
	// LC0..LC17
	{189,255}, {179,135}, {170,149}, {163,169}, {157,193}, {153,220}, {172,252}, {169,255}, {165,255},
	{128,26}, {128,60}, {128,94}, {128,128}, {128,162}, {128,196}, {255,255}, {148,255}, {161,255},
	// LD0..LD17
	{0,9}, {0,43}, {0,77}, {0,111}, {0,145}, {0,201}, {224,181}, {230,217}, {235,255},
	{189,128}, {200,131}, {210,141}, {218,159}, {255,255}, {207,238}, {211,255}, {218,255}, {255,255}
};
#elif defined (RGB_BACKLIGHT_ZEAL60) || defined (RGB_BACKLIGHT_M60_A)
const Point g_map_led_to_point[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA0..LA17
	{120,16}, {104,16}, {88,16}, {72,16}, {56,16}, {40,16}, {24,16}, {4,16}, {4,32},
	{128,0}, {112,0}, {96,0}, {80,0}, {64,0}, {48,0}, {32,0}, {16,0}, {0,0},
	// LB0..LB17
	{144,0}, {160,0}, {176,0}, {192,0}, {216,0}, {224,0}, {255,255}, {255,255}, {255,255},
	{136,16}, {152,16}, {168,16}, {184,16}, {200,16}, {220,16}, {255,255}, {255,255}, {255,255},
	// LC0..LC17
	{102,64}, {100,48}, {84,48}, {68,48}, {52,48}, {36,48}, {60,64}, {43,64}, {23,64},
	{108,32}, {92,32}, {76,32}, {60,32}, {44,32}, {28,32}, {20,48}, {2,48}, {3,64},
	// LD0..LD17
	{124,32}, {140,32}, {156,32}, {172,32}, {188,32}, {214,32}, {180,48}, {210,48}, {224,48},
	{116,48}, {132,48}, {148,48}, {164,48}, {144,64}, {161,64}, {181,64}, {201,64}, {221,64}
};
const Point g_map_led_to_point_polar[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA0..LA17
	{58,129}, {70,129}, {80,139}, {89,157}, {96,181}, {101,208}, {105,238}, {109,255}, {128,247}, {58,255},
	{64,255}, {70,255}, {75,255}, {80,255}, {85,255}, {89,255}, {93,255}, {96,255},
	// LB0..LB17
	{53,255}, {48,255}, {43,255}, {39,255}, {34,255}, {32,255}, {255,255}, {255,255}, {255,255},
	{48,139}, {39,157}, {32,181}, {27,208}, {23,238}, {19,255}, {255,255}, {255,255}, {255,255},
	// LC0..LC17
	{188,255}, {183,131}, {173,143}, {165,163}, {159,188}, {154,216}, {172,252}, {170,255}, {165,255},
	{128,9}, {128,46}, {128,82}, {128,119}, {128,155}, {128,192}, {150,244}, {147,255}, {161,255},
	// LD0..LD17
	{0,27}, {0,64}, {0,101}, {0,137}, {0,174}, {255,233}, {228,201}, {235,255}, {237,255},
	{195,128}, {206,136}, {215,152}, {222,175}, {205,234}, {209,255}, {214,255}, {219,255}, {223,255}
};
#elif defined (RGB_BACKLIGHT_HS60) && defined (HS60_ANSI)
const Point g_map_led_to_point[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA1..LA47
	{0,0}, {4,16}, {6,32}, {10,48}, {16,0}, {24,16}, {28,32}, {36,48}, {32,0}, {40,16}, {44,32}, {52,48},
	{48,0}, {56,16}, {60,32}, {68,48}, {64,0}, {72,16}, {76,32}, {84,48}, {80,0}, {88,16}, {92,32}, {100,48},
	{96,0}, {104,16}, {108,32}, {116,48}, {112,0}, {120,16}, {124,32}, {132,48}, {128,0}, {136,16}, {140,32},
	{148,48}, {144,0}, {152,16}, {156,32}, {164,48}, {160,0}, {168,16}, {172,32}, {180,48}, {176,0}, {184, 16}, {188,32},
	{255,255},// LA48 does not exist, dummy
	// LA49..LA50
	{192,0}, {200,16},
	{255,255},// LA51 does not exit, dummy
	// LA52..LA60
	{210,48}, {216,0}, {220,16}, {214,32}, {222,64}, {2,64}, {22,64}, {42,64}, {102,64},
	{255,255},// LA61 does not exit, dummy
	{162,64}, {182,64}, {202,64}
};
const Point g_map_led_to_point_polar[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA1..LA47
	{96,255}, {109,255}, {128,242}, {148,255}, {93,255}, {105,238}, {128,192}, {154,216}, {89,255}, {101,208}, {128,155}, {159,188},
	{85,255}, {96,181}, {128,119}, {165,163}, {81,255}, {89,157}, {128,82}, {173,143}, {75,255}, {81,139}, {128,46}, {183,131},
	{70,255}, {70,129}, {129,9}, {195,128}, {64,255}, {58,129}, {255,27}, {206,136}, {58,255}, {47,139}, {255,64}, {215,152},
	{53,255}, {39,157}, {255,101}, {222,175}, {47,255}, {32,181}, {255,137}, {228,201}, {43,255}, {27,208}, {255, 174},
	{255,255},// LA48 does not exist, dummy
	// LA49..LA50
	{39,255}, {23,238},
	{255,255},// LA51 does not exit, dummy
	// LA52..LA60
	{235,255}, {33,255}, {19,255}, {255,233}, {224,255}, {160,255}, {164,255}, {169,255}, {188,255},
	{255,255},// LA61 does not exit, dummy
	{209,255}, {215,255}, {220,255}
};
#elif defined (RGB_BACKLIGHT_HS60) && defined (HS60_HHKB)
const Point g_map_led_to_point[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA1..LA60
	{0,0}, {4,16}, {6,32}, {10,48}, {16,0}, {24,16}, {28,32}, {36,48}, {32,0}, {40,16}, {44,32}, {52,48},
	{48,0}, {56,16}, {60,32}, {68,48}, {64,0}, {72,16}, {76,32}, {84,48}, {80,0}, {88,16}, {92,32}, {100,48},
	{96,0}, {104,16}, {108,32}, {116,48}, {112,0}, {120,16}, {124,32}, {132,48}, {128,0}, {136,16}, {140,32},
	{148,48}, {144,0}, {152,16}, {156,32}, {164,48}, {160,0}, {168,16}, {172,32}, {180,48}, {176,0}, {184, 16}, {188,32},
	{224,0}, {192,0}, {200,16}, {202,48}, {224,48}, {208,0}, {220,16}, {214,32}, {220,64}, {4,64}, {24,64}, {44,64}, {112,64},
	{255,255}, {255,255}, // LA61..LA62 does not exit, dummy
	// LA63..LA64
	{180,64}, {200,64}
};
const Point g_map_led_to_point_polar[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA1..LA60
	{96,255}, {109,255}, {128,242}, {148,255}, {93,255}, {105,238}, {128,192}, {154,216}, {89,255}, {101,208}, {128,155}, {159,188},
	{85,255}, {96,181}, {128,119}, {165,163}, {81,255}, {89,157}, {128,82}, {173,143}, {75,255}, {81,139}, {128,46}, {183,131},
	{70,255}, {70,129}, {129,9}, {195,128}, {64,255}, {58,129}, {255,27}, {206,136}, {58,255}, {47,139}, {255,64}, {215,152},
	{53,255}, {39,157}, {255,101}, {222,175}, {47,255}, {32,181}, {255,137}, {228,201}, {43,255}, {27,208}, {255, 174}, {32,255},
	{39,255}, {23,238}, {233,242}, {237,255}, {35,255}, {19,255}, {255,233}, {223,255}, {161,255}, {165,255}, {170,255}, {192,255},
	{255,255}, {255,255}, // LA61..LA62 does not exit, dummy
	// LA63..LA64
	{214,255}, {219,255}
};
#elif defined (RGB_BACKLIGHT_HS60) //HS60_ISO
const Point g_map_led_to_point[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA1..LA50
	{0,0}, {4,16}, {6,32}, {2,48}, {16,0}, {24,16}, {28,32}, {36,48}, {32,0}, {40,16}, {44,32}, {52,48}, {48,0},
	{56,16}, {60,32}, {68,48}, {64,0}, {72,16}, {76,32}, {84,48}, {80,0}, {88,16}, {92,32}, {100,48}, {96,0}, {104,16}, 
	{108,32}, {116,48}, {112,0}, {120,16}, {124,32}, {132,48}, {128,0}, {136,16}, {140,32}, {148,48}, {144,0}, {152,16},
	{156,32}, {164,48}, {160,0}, {168,16}, {172,32}, {180,48}, {176,0}, {184, 16}, {188,32}, {20,48}, {192,0}, {200,16},
	{255,255},// LA51 does not exit, dummy
	// LA52..LA60
	{210,48}, {216,0}, {220,16}, {222,24}, {222,64}, {2,64}, {22,64}, {42,64}, {102,64},
	{255,255},// LA61 does not exit, dummy
	{162,64}, {182,64}, {202,64}
};
const Point g_map_led_to_point_polar[BACKLIGHT_LED_COUNT] PROGMEM = {
	// LA1..LA50
	{96,255}, {109,255}, {128,242}, {147,255}, {93,255}, {105,238}, {128,192}, {154,216}, {89,255}, {101,208}, {128,155}, {159,188}, {85,255},
	{96,181}, {128,119}, {165,163}, {81,255}, {89,157}, {128,82}, {173,143}, {75,255}, {81,139}, {128,46}, {183,131}, {70,255}, {70,129},
	{129,9}, {195,128}, {64,255}, {58,129}, {255,27}, {206,136}, {58,255}, {47,139}, {255,64}, {215,152}, {53,255}, {39,157}, {255,101}, 
	{222,175}, {47,255}, {32,181}, {255,137}, {228,201}, {43,255}, {27,208}, {255, 174}, {150,246}, {39,255}, {23,238},
	{255,255},// LA51 does not exit, dummy
	// LA52..LA60
	{235,255}, {33,255}, {19,255}, {10,255}, {224,255}, {160,255}, {164,255}, {169,255}, {188,255},
	{255,255},// LA61 does not exit, dummy
	{209,255}, {215,255}, {220,255}
};
#elif defined (RGB_BACKLIGHT_M6_B)
// M6-B is really simple:
// 0 3 5
// 1 2 4
const Point g_map_led_to_point[BACKLIGHT_LED_COUNT] PROGMEM = {
	{0,0}, {0,16}, {16,16}, {16,0}, {32,16}, {32,0}
};
const Point g_map_led_to_point_polar[BACKLIGHT_LED_COUNT] PROGMEM = {
	{160,255}, {96,255}, {77,255}, {179,255}, {51,255}, {205,255}
};
#endif

// This may seem counter-intuitive, but it's quite flexible.
// For each LED, get it's position to decide what color to make it.
// This solves the issue of LEDs (and switches) not aligning to a grid,
// or having a large "bitmap" and sampling them.
void map_led_to_point( uint8_t index, Point *point )
{
	// Slightly messy way to get Point structs out of progmem.
	uint8_t *addr = (uint8_t*)&g_map_led_to_point[index];
	point->x = pgm_read_byte(addr);
	point->y = pgm_read_byte(addr+1);

#if defined (RGB_BACKLIGHT_M6_B)
	return;
#endif

	switch (index)
	{
		case 18+4: // LB4A
			if ( g_config.use_split_backspace )
				point->x -= 8;
			break;
#if defined (RGB_BACKLIGHT_ZEAL60)
		case 18+14: // LB14A
			if ( g_config.use_iso_enter )
				point->y += 8; // extremely pedantic
			break;
		case 54+5: // LD5A
			if ( !g_config.use_iso_enter )
				point->x -= 10;
			break;
		case 36+16: // LC16A
			if ( !g_config.use_split_left_shift )
				point->x += 8;
			break;
#endif
#if defined (RGB_BACKLIGHT_ZEAL60) || defined (RGB_BACKLIGHT_M60_A)
		case 36+0: // LC0A
			if ( g_config.use_7u_spacebar )
				point->x += 10;
			break;
		case 36+6: // LC6A
			if ( g_config.use_7u_spacebar )
				point->x += 4;
			break;
		case 54+7: // LD7A
			if ( !g_config.use_split_right_shift )
				point->x -= 8;
			break;
#endif
	}
}

void map_led_to_point_polar( uint8_t index, Point *point )
{
	// Slightly messy way to get Point structs out of progmem.
	uint8_t *addr = (uint8_t*)&g_map_led_to_point_polar[index];
	point->x = pgm_read_byte(addr);
	point->y = pgm_read_byte(addr+1);
}

//
// Maps switch matrix coordinate (row,col) to LED index
//


#if defined (RGB_BACKLIGHT_ZEAL65)
// Note: Left spacebar stab is at 4,2 (LC7)
// Right spacebar stab is at 4,9 (D14)
//
// A17, A16, A15, A14, A13, A12, A11, A10,  A9,  B0,  B1,  B2,  B3,  B4,  B6
//  A7,  A6,  A5,  A4,  A3,  A2,  A1,  A0,  B9, B10, B11, B12, B13, B14,  B7
//  A8, C14, C13, C12, C11, C10,  C9,  D0,  D1,  D2,  D3,  D4,  D5,  B5,  B8
// C16, C15,  C5,  C4,  C3,  C2,  C1,  D9, D10, D11, D12,  D6,  D7,  D8, B15
// C17,  C8,  C7, ---, ---, ---, ---,  C0, ---, D14, D15, D16, D17, B17, B16
const uint8_t g_map_row_column_to_led[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
	{  0+17,  0+16,  0+15,  0+14,  0+13,  0+12,  0+11,  0+10,   0+9,  18+0,  18+1,  18+2,  18+3,  18+4,  18+6 },
	{   0+7,   0+6,   0+5,   0+4,   0+3,   0+2,   0+1,   0+0,  18+9, 18+10, 18+11, 18+12, 18+13, 18+14,  18+7 },
	{   0+8, 36+14, 36+13, 36+12, 36+11, 36+10,  36+9,  54+0,  54+1,  54+2,  54+3,  54+4,  54+5,  18+5,  18+8 },
	{ 36+16, 36+15,  36+5,  36+4,  36+3,  36+2,  36+1,  54+9, 54+10, 54+11, 54+12,  54+6,  54+7,  54+8, 18+15 },
	{ 36+17,  36+8,  36+7,   255,   255,   255,   255,  36+0,  255,  54+14, 54+15, 54+16, 54+17, 18+17, 18+16 }
};
#elif defined(RGB_BACKLIGHT_KOYU)
// Note: Left spacebar stab is at 4,4 (LC6)
// Right spacebar stab is at 4,10 (D14)
//
// A17, A16, A15, A14, A13, A12, A11, A10,  A9,  B0,  B1,  B2,  B3,  B4,  B6
//  A7,  A6,  A5,  A4,  A3,  A2,  A1,  A0,  B9, B10, B11, B12, B13, B14,  B7
//  A8, C14, C13, C12, C11, C10,  C9,  D0,  D1,  D2,  D3,  D4,  D5,  B5,  B8
// C16, C15,  C5,  C4,  C3,  C2,  C1,  D9, D10, D11, D12,  D6,  D7,  D8, B15
// C17,  C8,  C7,  C6, ---, ---, ---,  C0, ---, ---, D14, D15, D16, B17, B16
const uint8_t g_map_row_column_to_led[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
	{  0+17,  0+16,  0+15,  0+14,  0+13,  0+12,  0+11,  0+10,   0+9,  18+0,  18+1,  18+2,  18+3,  18+4,  18+6 },
	{   0+7,   0+6,   0+5,   0+4,   0+3,   0+2,   0+1,   0+0,  18+9, 18+10, 18+11, 18+12, 18+13, 18+14,  18+7 },
	{   0+8, 36+14, 36+13, 36+12, 36+11, 36+10,  36+9,  54+0,  54+1,  54+2,  54+3,  54+4,  54+5,  18+5,  18+8 },
	{ 36+16, 36+15,  36+5,  36+4,  36+3,  36+2,  36+1,  54+9, 54+10, 54+11, 54+12,  54+6,  54+7,  54+8, 18+15 },
	{ 36+17,  36+8,  36+7,  36+6,   255,   255,   255,  36+0,  255,    255, 54+14, 54+15, 54+16, 18+17, 18+16 }
};
#elif defined (RGB_BACKLIGHT_ZEAL60) || defined (RGB_BACKLIGHT_M60_A)
// Note: Left spacebar stab is at 4,3 (LC6)
// Right spacebar stab is at 4,9 (LD13) or 4,10 (LD14)
//
// A17, A16, A15, A14, A13, A12, A11, A10,  A9,  B0,  B1,  B2,  B3,  B4,
//  A7,  A6,  A5,  A4,  A3,  A2,  A1,  A0,  B9, B10, B11, B12, B13, B14,
//  A8, C14, C13, C12, C11, C10,  C9,  D0,  D1,  D2,  D3,  D4,  D5,  B5,
// C16, C15,  C5,  C4,  C3,  C2,  C1,  D9, D10, D11, D12,  D6,  D7,  D8,
// C17,  C8,  C7,  C6, ---, ---, ---,  C0, ---, D13, D14, D15, D16, D17,
const uint8_t g_map_row_column_to_led[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
	{  0+17,  0+16,  0+15,  0+14,  0+13,  0+12,  0+11,  0+10,   0+9,  18+0,  18+1,  18+2,  18+3,  18+4 },
	{   0+7,   0+6,   0+5,   0+4,   0+3,   0+2,   0+1,   0+0,  18+9, 18+10, 18+11, 18+12, 18+13, 18+14 },
	{   0+8, 36+14, 36+13, 36+12, 36+11, 36+10,  36+9,  54+0,  54+1,  54+2,  54+3,  54+4,  54+5,  18+5 },
	{ 36+16, 36+15,  36+5,  36+4,  36+3,  36+2,  36+1,  54+9, 54+10, 54+11, 54+12,  54+6,  54+7,  54+8 },
	{ 36+17,  36+8,  36+7,  36+6,   255,   255,   255,  36+0,  255,  54+13, 54+14, 54+15, 54+16, 54+17 }
};
#elif defined (RGB_BACKLIGHT_HS60) && defined (HS60_ANSI)
//
// LA1,  LA5,  LA9,  LA13, LA17, LA21, LA25, LA29, LA33, LA37, LA41, LA45, LA49, LA53,
// LA2,  LA6,  LA10, LA14, LA18, LA22, LA26, LA30, LA34, LA38, LA42, LA46, LA50,  ---,
// LA3,  LA7,  LA11, LA15, LA19, LA23, LA27, LA31, LA35, LA39, LA43, LA47, LA54, LA55,
// LA4,  ---,   LA8, LA12, LA16, LA20, LA24, LA28, LA32, LA36, LA40, LA44,  ---, LA52,
// LA57, LA58, LA59,  ---,  ---,  ---, LA60,  ---,  ---,  ---, LA62, LA63, LA64, LA56
const uint8_t g_map_row_column_to_led[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
	{  1-1,  5-1,  9-1, 13-1, 17-1, 21-1, 25-1, 29-1, 33-1, 37-1, 41-1, 45-1, 49-1, 53-1 },
	{  2-1,  6-1, 10-1, 14-1, 18-1, 22-1, 26-1, 30-1, 34-1, 38-1, 42-1, 46-1, 50-1,  255 },
	{  3-1,  7-1, 11-1, 15-1, 19-1, 23-1, 27-1, 31-1, 35-1, 39-1, 43-1, 47-1, 54-1, 55-1 },
	{  4-1,  255,  8-1, 12-1, 16-1, 20-1, 24-1, 28-1, 32-1, 36-1, 40-1, 44-1,  255, 52-1 },
	{ 57-1, 58-1, 59-1,  255,  255,  255, 60-1,  255,  255,  255, 62-1, 63-1, 64-1, 56-1 }
};
#elif defined (RGB_BACKLIGHT_HS60) && defined (HS60_HHKB)
//
// LA1,  LA5,   LA9, LA13, LA17, LA21, LA25, LA29, LA33, LA37, LA41, LA45, LA49, LA53,
// LA2,  LA6,  LA10, LA14, LA18, LA22, LA26, LA30, LA34, LA38, LA42, LA46, LA50, LA48,
// LA3,  LA7,  LA11, LA15, LA19, LA23, LA27, LA31, LA35, LA39, LA43, LA47, LA54, LA55,
// LA4,  ---,   LA8, LA12, LA16, LA20, LA24, LA28, LA32, LA36, LA40, LA44, LA51, LA52,
// LA57, LA58, LA59,  ---,  ---,  ---, LA60,  ---,  ---,  ---,  ---, LA63, LA64, LA56
const uint8_t g_map_row_column_to_led[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
	{  1-1,  5-1,  9-1, 13-1, 17-1, 21-1, 25-1, 29-1, 33-1, 37-1, 41-1, 45-1, 49-1, 53-1 },
	{  2-1,  6-1, 10-1, 14-1, 18-1, 22-1, 26-1, 30-1, 34-1, 38-1, 42-1, 46-1, 50-1, 48-1 },
	{  3-1,  7-1, 11-1, 15-1, 19-1, 23-1, 27-1, 31-1, 35-1, 39-1, 43-1, 47-1, 54-1, 55-1 },
	{  4-1,  255,  8-1, 12-1, 16-1, 20-1, 24-1, 28-1, 32-1, 36-1, 40-1, 44-1, 51-1, 52-1 },
	{ 57-1, 58-1, 59-1,  255,  255,  255, 60-1,  255,  255,  255,  255, 63-1, 64-1, 56-1 }
};
#elif defined (RGB_BACKLIGHT_HS60) //HS60_ISO
//
// LA1,  LA5,   LA9, LA13, LA17, LA21, LA25, LA29, LA33, LA37, LA41, LA45, LA49, LA53,
// LA2,  LA6,  LA10, LA14, LA18, LA22, LA26, LA30, LA34, LA38, LA42, LA46, LA50,  ---,
// LA3,  LA7,  LA11, LA15, LA19, LA23, LA27, LA31, LA35, LA39, LA43, LA47, LA54, LA55,
// LA4,  LA48,  LA8, LA12, LA16, LA20, LA24, LA28, LA32, LA36, LA40, LA44,  ---, LA52,
// LA57, LA58, LA59,  ---,  ---,  ---, LA60,  ---,  ---,  ---, LA62, LA63, LA64, LA56
const uint8_t g_map_row_column_to_led[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
	{  1-1,  5-1,  9-1, 13-1, 17-1, 21-1, 25-1, 29-1, 33-1, 37-1, 41-1, 45-1, 49-1, 53-1 },
	{  2-1,  6-1, 10-1, 14-1, 18-1, 22-1, 26-1, 30-1, 34-1, 38-1, 42-1, 46-1, 50-1,  255 },
	{  3-1,  7-1, 11-1, 15-1, 19-1, 23-1, 27-1, 31-1, 35-1, 39-1, 43-1, 47-1, 54-1, 55-1 },
	{  4-1, 48-1,  8-1, 12-1, 16-1, 20-1, 24-1, 28-1, 32-1, 36-1, 40-1, 44-1,  255, 52-1 },
	{ 57-1, 58-1, 59-1,  255,  255,  255, 60-1,  255,  255,  255, 62-1, 63-1, 64-1, 56-1 }
};
#elif defined (RGB_BACKLIGHT_M6_B)
// M6-B is really simple:
// 0 3 5
// 1 2 4
const uint8_t g_map_row_column_to_led[MATRIX_ROWS][MATRIX_COLS] PROGMEM = {
	{     0,     3,     5,     1,     2,     4 }
};
#endif

void map_row_column_to_led( uint8_t row, uint8_t column, uint8_t *led )
{
	*led = 255;
	if ( row < MATRIX_ROWS && column < MATRIX_COLS )
	{
		*led = pgm_read_byte(&g_map_row_column_to_led[row][column]);
	}
}

void backlight_update_pwm_buffers(void)
{
#if defined (RGB_BACKLIGHT_M6_B)
	IS31FL3218_update_pwm_buffers();
#elif defined (RGB_BACKLIGHT_HS60)
	IS31FL3733_update_pwm_buffers( ISSI_ADDR_1, ISSI_ADDR_2 );
	IS31FL3733_update_led_control_registers( ISSI_ADDR_1, ISSI_ADDR_2 );
#else
	IS31FL3731_update_pwm_buffers( ISSI_ADDR_1, ISSI_ADDR_2 );
	IS31FL3731_update_led_control_registers( ISSI_ADDR_1, ISSI_ADDR_2 );
#endif
}

void backlight_set_color( int index, uint8_t red, uint8_t green, uint8_t blue )
{
#if defined (RGB_BACKLIGHT_M6_B)
	IS31FL3218_set_color( index, red, green, blue );
#elif defined (RGB_BACKLIGHT_HS60)
	IS31FL3733_set_color( index, red, green, blue );
#else
	IS31FL3731_set_color( index, red, green, blue );
#endif
}

void backlight_set_color_all( uint8_t red, uint8_t green, uint8_t blue )
{
#if defined (RGB_BACKLIGHT_M6_B)
	IS31FL3218_set_color_all( red, green, blue );
#elif defined (RGB_BACKLIGHT_HS60)
	IS31FL3733_set_color_all( red, green, blue );
#else
	IS31FL3731_set_color_all( red, green, blue );
#endif
}

void backlight_set_key_hit(uint8_t row, uint8_t column)
{
	uint8_t led;
	map_row_column_to_led(row,column,&led);
	g_key_hit[led] = 0;

	g_any_key_hit = 0;
}

#if !defined(RGB_BACKLIGHT_HS60)
// This is (F_CPU/1024) / 20 Hz
// = 15625 Hz / 20 Hz
// = 781
#define TIMER3_TOP 781

void backlight_timer_init(void)
{
	static uint8_t backlight_timer_is_init = 0;
	if ( backlight_timer_is_init )
	{
		return;
	}
	backlight_timer_is_init = 1;

	// Timer 3 setup
	TCCR3B = _BV(WGM32) | 			// CTC mode OCR3A as TOP
			 _BV(CS32) | _BV(CS30); // prescale by /1024
	// Set TOP value
	uint8_t sreg = SREG;
	cli();

	OCR3AH = (TIMER3_TOP >> 8) & 0xff;
	OCR3AL = TIMER3_TOP & 0xff;
	SREG = sreg;
}

void backlight_timer_enable(void)
{
	TIMSK3 |= _BV(OCIE3A);
}

void backlight_timer_disable(void)
{
	TIMSK3 &= ~_BV(OCIE3A);
}
#else //STM32, use GPT with TIM4. Enable in halconf.h
static void gpt_backlight_timer_task(GPTDriver *gptp);
// Timer setup at 200Khz, callback at 10k ticks = 20Hz
static GPTConfig gpt4cfg1 = {
	.frequency = 200000U,
	.callback  = gpt_backlight_timer_task
};

void backlight_timer_init(void)
{
	gptStart(&GPTD4, &gpt4cfg1);
}

void backlight_timer_enable(void)
{
	gptStartContinuous(&GPTD4, 10000);
}

void backlight_timer_disable(void)
{
	gptStopTimer(&GPTD4);
}
#endif //!defined(RGB_BACKLIGHT_HS60)

void backlight_set_suspend_state(bool state)
{
	g_suspend_state = state;
}

void backlight_set_indicator_state(uint8_t state)
{
	g_indicator_state = state;
}

void backlight_effect_rgb_test(void)
{
	// Mask out bits 4 and 5
	// This 2-bit value will stay the same for 16 ticks.
	switch ( (g_tick & 0x30) >> 4 )
	{
		case 0:
		{
			backlight_set_color_all( 255, 0, 0 );
			break;
		}
		case 1:
		{
			backlight_set_color_all( 0, 255, 0 );
			break;
		}
		case 2:
		{
			backlight_set_color_all( 0, 0, 255 );
			break;
		}
		case 3:
		{
			backlight_set_color_all( 255, 255, 255 );
			break;
		}
	}
}

#if defined(RGB_DEBUGGING_ONLY)
// This tests the LEDs
// Note that it will change the LED control registers
// in the LED drivers, and leave them in an invalid
// state for other backlight effects.
// ONLY USE THIS FOR TESTING LEDS!
void backlight_effect_single_LED_test(void)
{
	static uint8_t color = 0; // 0,1,2 for R,G,B
	static uint8_t row = 0;
	static uint8_t column = 0;

	static uint8_t tick = 0;
	tick++;

	if ( tick > 2 )
	{
		tick = 0;
		column++;
	}
	if ( column > 14 )
	{
		column = 0;
		row++;
	}
	if ( row > 4 )
	{
		row = 0;
		color++;
	}
	if ( color > 2 )
	{
		color = 0;
	}

	uint8_t led;
	map_row_column_to_led( row, column, &led );
	backlight_set_color_all( 255, 255, 255 );
	backlight_test_led( led, color==0, color==1, color==2 );
}
#endif // defined(RGB_DEBUGGING_ONLY)

// All LEDs off
void backlight_effect_all_off(void)
{
	backlight_set_color_all( 0, 0, 0 );
}

// Solid color
void backlight_effect_solid_color(void)
{
	HSV hsv = { .h = g_config.color_1.h, .s = g_config.color_1.s, .v = g_config.brightness };
	RGB rgb = hsv_to_rgb( hsv );
	backlight_set_color_all( rgb.r, rgb.g, rgb.b );
}

// alphas = color1, mods = color2
void backlight_effect_alphas_mods(void)
{
	RGB rgb1 = hsv_to_rgb( (HSV){ .h = g_config.color_1.h, .s = g_config.color_1.s, .v = g_config.brightness } );
	RGB rgb2 = hsv_to_rgb( (HSV){ .h = g_config.color_2.h, .s = g_config.color_2.s, .v = g_config.brightness } );

	for ( int row = 0; row < MATRIX_ROWS; row++ )
	{
		for ( int column = 0; column < MATRIX_COLS; column++ )
		{
			uint8_t index;
			map_row_column_to_led( row, column, &index );
			if ( index < BACKLIGHT_LED_COUNT )
			{
				if ( ( g_config.alphas_mods[row] & (1<<column) ) == 0 )
				{
					backlight_set_color( index, rgb1.r, rgb1.g, rgb1.b );
				}
				else
				{
					backlight_set_color( index, rgb2.r, rgb2.g, rgb2.b );
				}
			}
		}
	}
}

void backlight_effect_gradient_up_down(void)
{
	int16_t h1 = g_config.color_1.h;
	int16_t h2 = g_config.color_2.h;
	int16_t deltaH = h2 - h1;

	// Take the shortest path between hues
	if ( deltaH > 127 )
	{
		deltaH -= 256;
	}
	else if ( deltaH < -127 )
	{
		deltaH += 256;
	}
	// Divide delta by 4, this gives the delta per row
	deltaH /= 4;

	int16_t s1 = g_config.color_1.s;
	int16_t s2 = g_config.color_2.s;
	int16_t deltaS = ( s2 - s1 ) / 4;

	HSV hsv = { .h = 0, .s = 255, .v = g_config.brightness };
	RGB rgb;
	Point point;
	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		map_led_to_point( i, &point );
		// The y range will be 0..64, map this to 0..4
		uint8_t y = (point.y>>4);
		// Relies on hue being 8-bit and wrapping
		hsv.h = g_config.color_1.h + ( deltaH * y );
		hsv.s = g_config.color_1.s + ( deltaS * y );
		rgb = hsv_to_rgb( hsv );
		backlight_set_color( i, rgb.r, rgb.g, rgb.b );
	}
}

void backlight_effect_raindrops(bool initialize)
{
	int16_t h1 = g_config.color_1.h;
	int16_t h2 = g_config.color_2.h;
	int16_t deltaH = h2 - h1;
	deltaH /= 4;

	// Take the shortest path between hues
	if ( deltaH > 127 )
	{
		deltaH -= 256;
	}
	else if ( deltaH < -127 )
	{
		deltaH += 256;
	}

	int16_t s1 = g_config.color_1.s;
	int16_t s2 = g_config.color_2.s;
	int16_t deltaS = ( s2 - s1 ) / 4;

	HSV hsv;
	RGB rgb;

	// Change one LED every tick
	uint8_t led_to_change = ( g_tick & 0x000 ) == 0 ? rand() % BACKLIGHT_LED_COUNT : 255;

	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		// If initialize, all get set to random colors
		// If not, all but one will stay the same as before.
		if ( initialize || i == led_to_change )
		{
			hsv.h = h1 + ( deltaH * ( rand() & 0x03 ) );
			hsv.s = s1 + ( deltaS * ( rand() & 0x03 ) );
			// Override brightness with global brightness control
			hsv.v = g_config.brightness;;

			rgb = hsv_to_rgb( hsv );
			backlight_set_color( i, rgb.r, rgb.g, rgb.b );
		}
	}
}

void backlight_effect_cycle_all(void)
{
	uint8_t offset = ( g_tick << g_config.effect_speed ) & 0xFF;

	// Relies on hue being 8-bit and wrapping
	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		uint16_t offset2 = g_key_hit[i]<<2;
		// stabilizer LEDs use spacebar hits
		if ( i == 36+6 || i == 54+13 || // LC6, LD13
				( g_config.use_7u_spacebar && i == 54+14 ) ) // LD14
		{
			offset2 = g_key_hit[36+0]<<2;
		}
		offset2 = (offset2<=63) ? (63-offset2) : 0;

		HSV hsv = { .h = offset+offset2, .s = 255, .v = g_config.brightness };
		RGB rgb = hsv_to_rgb( hsv );
		backlight_set_color( i, rgb.r, rgb.g, rgb.b );
	}
}

void backlight_effect_cycle_left_right(void)
{
	uint8_t offset = ( g_tick << g_config.effect_speed ) & 0xFF;
	HSV hsv = { .h = 0, .s = 255, .v = g_config.brightness };
	RGB rgb;
	Point point;
	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		uint16_t offset2 = g_key_hit[i]<<2;
		// stabilizer LEDs use spacebar hits
		if ( i == 36+6 || i == 54+13 || // LC6, LD13
				( g_config.use_7u_spacebar && i == 54+14 ) ) // LD14
		{
			offset2 = g_key_hit[36+0]<<2;
		}
		offset2 = (offset2<=63) ? (63-offset2) : 0;

		map_led_to_point( i, &point );
		// Relies on hue being 8-bit and wrapping
		hsv.h = point.x + offset + offset2;
		rgb = hsv_to_rgb( hsv );
		backlight_set_color( i, rgb.r, rgb.g, rgb.b );
	}
}

void backlight_effect_cycle_up_down(void)
{
	uint8_t offset = ( g_tick << g_config.effect_speed ) & 0xFF;
	HSV hsv = { .h = 0, .s = 255, .v = g_config.brightness };
	RGB rgb;
	Point point;
	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		uint16_t offset2 = g_key_hit[i]<<2;
		// stabilizer LEDs use spacebar hits
		if ( i == 36+6 || i == 54+13 || // LC6, LD13
				( g_config.use_7u_spacebar && i == 54+14 ) ) // LD14
		{
			offset2 = g_key_hit[36+0]<<2;
		}
		offset2 = (offset2<=63) ? (63-offset2) : 0;

		map_led_to_point( i, &point );
		// Relies on hue being 8-bit and wrapping
		hsv.h = point.y + offset + offset2;
		rgb = hsv_to_rgb( hsv );
		backlight_set_color( i, rgb.r, rgb.g, rgb.b );
	}
}

void backlight_effect_jellybean_raindrops( bool initialize )
{
	HSV hsv;
	RGB rgb;

	// Change one LED every tick
	uint8_t led_to_change = ( g_tick & 0x000 ) == 0 ? rand() % BACKLIGHT_LED_COUNT : 255;

	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		// If initialize, all get set to random colors
		// If not, all but one will stay the same as before.
		if ( initialize || i == led_to_change )
		{
			hsv.h = rand() & 0xFF;
			hsv.s = rand() & 0xFF;
			// Override brightness with global brightness control
			hsv.v = g_config.brightness;;

			rgb = hsv_to_rgb( hsv );
			backlight_set_color( i, rgb.r, rgb.g, rgb.b );
		}
	}
}

void backlight_effect_cycle_radial1(void)
{
	uint8_t offset = ( g_tick << g_config.effect_speed ) & 0xFF;
	HSV hsv = { .h = 0, .s = 255, .v = g_config.brightness };
	RGB rgb;
	Point point;
	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		map_led_to_point_polar( i, &point );
		// Relies on hue being 8-bit and wrapping
		hsv.h = point.x + offset;
		hsv.s = point.y;
		rgb = hsv_to_rgb( hsv );
		backlight_set_color( i, rgb.r, rgb.g, rgb.b );
	}
}

void backlight_effect_cycle_radial2(void)
{
	uint8_t offset = ( g_tick << g_config.effect_speed ) & 0xFF;

	HSV hsv = { .h = 0, .s = g_config.color_1.s, .v = g_config.brightness };
	RGB rgb;
	Point point;
	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		map_led_to_point_polar( i, &point );
		uint8_t offset2 = offset + point.x;
		if ( offset2 & 0x80 )
		{
			offset2 = ~offset2;
		}
		offset2 = offset2 >> 2;
		hsv.h = g_config.color_1.h + offset2;
		hsv.s = 127 + ( point.y >> 1 );
		rgb = hsv_to_rgb( hsv );
		backlight_set_color( i, rgb.r, rgb.g, rgb.b );
	}
}

#if defined(RGB_BACKLIGHT_M6_B)
void backlight_effect_custom_colors(void)
{
	RGB rgb;
	for ( uint8_t i = 0; i < 6; i++ )
	{
		HSV hsv = { .h = g_config.custom_color[i].h, .s = g_config.custom_color[i].s, .v = g_config.brightness };
		rgb = hsv_to_rgb( hsv );
		uint8_t led;
		map_row_column_to_led( 0, i, &led );
		backlight_set_color( led, rgb.r, rgb.g, rgb.b );
	}
}
#endif

void backlight_effect_indicators_set_colors( uint8_t index, HS color )
{
	HSV hsv = { .h = color.h, .s = color.s, .v = g_config.brightness };
	RGB rgb = hsv_to_rgb( hsv );
	if ( index == 254 )
	{
		backlight_set_color_all( rgb.r, rgb.g, rgb.b );
	}
	else
	{
		backlight_set_color( index, rgb.r, rgb.g, rgb.b );

		// If the spacebar LED is the indicator,
		// do the same for the spacebar stabilizers
		if ( index == 36+0 ) // LC0
		{
#if defined (RGB_BACKLIGHT_ZEAL65)
			backlight_set_color( 36+7, rgb.r, rgb.g, rgb.b ); // LC7
			backlight_set_color( 54+14, rgb.r, rgb.g, rgb.b ); // LD14
#elif defined (RGB_BACKLIGHT_KOYU)
			backlight_set_color( 36+6, rgb.r, rgb.g, rgb.b ); // LC6
			backlight_set_color( 54+14, rgb.r, rgb.g, rgb.b ); // LD14
#elif defined (RGB_BACKLIGHT_ZEAL60) || defined (RGB_BACKLIGHT_M60_A)
			backlight_set_color( 36+6, rgb.r, rgb.g, rgb.b ); // LC6
			backlight_set_color( 54+13, rgb.r, rgb.g, rgb.b ); // LD13
			if ( g_config.use_7u_spacebar )
			{
				backlight_set_color( 54+14, rgb.r, rgb.g, rgb.b ); // LD14
			}
#endif
		}
	}
}

// This runs after another backlight effect and replaces
// colors already set
void backlight_effect_indicators(void)
{
	if ( g_config.caps_lock_indicator.index != 255 &&
			( g_indicator_state & (1<<USB_LED_CAPS_LOCK) ) )
	{
		backlight_effect_indicators_set_colors( g_config.caps_lock_indicator.index, g_config.caps_lock_indicator.color );
	}

	// This if/else if structure allows higher layers to
	// override lower ones. If we set layer 3's indicator
	// to none, then it will NOT show layer 2 or layer 1
	// indicators, even if those layers are on via the
	// MO13/MO23 Fn combo magic.
	//
	// Basically we want to handle the case where layer 3 is
	// still the backlight configuration layer and we don't
	// want "all LEDs" indicators hiding the backlight effect,
	// but still allow end users to do whatever they want.
	if ( IS_LAYER_ON(3) )
	{
		if ( g_config.layer_3_indicator.index != 255 )
		{
			backlight_effect_indicators_set_colors( g_config.layer_3_indicator.index, g_config.layer_3_indicator.color );
		}
	}
	else if ( IS_LAYER_ON(2) )
	{
		if ( g_config.layer_2_indicator.index != 255 )
		{
			backlight_effect_indicators_set_colors( g_config.layer_2_indicator.index, g_config.layer_2_indicator.color );
		}
	}
	else if ( IS_LAYER_ON(1) )
	{
		if ( g_config.layer_1_indicator.index != 255 )
		{
			backlight_effect_indicators_set_colors( g_config.layer_1_indicator.index, g_config.layer_1_indicator.color );
		}
	}
}

#if !defined(RGB_BACKLIGHT_HS60)
ISR(TIMER3_COMPA_vect)
#else //STM32 interrupt
static void gpt_backlight_timer_task(GPTDriver *gptp)
#endif
{
	// delay 1 second before driving LEDs or doing anything else
	static uint8_t startup_tick = 0;
	if ( startup_tick < 20 )
	{
		startup_tick++;
		return;
	}

	g_tick++;

	if ( g_any_key_hit < 0xFFFFFFFF )
	{
		g_any_key_hit++;
	}

	for ( int led = 0; led < BACKLIGHT_LED_COUNT; led++ )
	{
		if ( g_key_hit[led] < 255 )
		{
			g_key_hit[led]++;
		}
	}

	// Factory default magic value
	if ( g_config.effect == 255 )
	{
		backlight_effect_rgb_test();
		return;
	}

	// Ideally we would also stop sending zeros to the LED driver PWM buffers
	// while suspended and just do a software shutdown. This is a cheap hack for now.
	bool suspend_backlight = ((g_suspend_state && g_config.disable_when_usb_suspended) ||
			(g_config.disable_after_timeout > 0 && g_any_key_hit > g_config.disable_after_timeout * 60 * 20));
	uint8_t effect = suspend_backlight ? 0 : g_config.effect;

	// Keep track of the effect used last time,
	// detect change in effect, so each effect can
	// have an optional initialization.
	static uint8_t effect_last = 255;
	bool initialize = effect != effect_last;
	effect_last = effect;

	// this gets ticked at 20 Hz.
	// each effect can opt to do calculations
	// and/or request PWM buffer updates.
	switch ( effect )
	{
		case 0:
			backlight_effect_all_off();
			break;
		case 1:
			backlight_effect_solid_color();
			break;
		case 2:
#if defined(RGB_BACKLIGHT_M6_B)
			backlight_effect_custom_colors();
#else
			backlight_effect_alphas_mods();
#endif
			break;
		case 3:
			backlight_effect_gradient_up_down();
			break;
		case 4:
			backlight_effect_raindrops( initialize );
			break;
		case 5:
			backlight_effect_cycle_all();
			break;
		case 6:
			backlight_effect_cycle_left_right();
			break;
		case 7:
			backlight_effect_cycle_up_down();
			break;
		case 8:
			backlight_effect_jellybean_raindrops( initialize );
			break;
		case 9:
			backlight_effect_cycle_radial1();
			break;
		case 10:
			backlight_effect_cycle_radial2();
			break;
		default:
			backlight_effect_all_off();
			break;
	}

	if ( ! suspend_backlight )
	{
#if !defined(RGB_BACKLIGHT_M6_B)
		backlight_effect_indicators();
#endif
	}
}

void backlight_set_indicator_index( uint8_t *index, uint8_t row, uint8_t column )
{
	if ( row >= MATRIX_ROWS )
	{
		// Special value, 255=none, 254=all
		*index = row;
	}
	else
	{
		map_row_column_to_led( row, column, index );
	}
}

void backlight_get_indicator_row_col( uint8_t index, uint8_t *row, uint8_t *column )
{
	if ( index == 255 || index == 254 )
	{
		// Special value, 255=none, 254=all
		*row = index;
		*column = 0;
		return;
	}
	for ( uint8_t r = 0; r < MATRIX_ROWS; r++ )
	{
		for ( uint8_t c = 0; c < MATRIX_COLS; c++ )
		{
			uint8_t i = 255;
			map_row_column_to_led( r, c, &i );
			if ( i == index )
			{
				*row = r;
				*column = c;
				return;
			}
		}
	}
}

// Some helpers for setting/getting HSV
void _set_color( HS *color, uint8_t *data )
{
	color->h = data[0];
	color->s = data[1];
}

void _get_color( HS *color, uint8_t *data )
{
	data[0] = color->h;
	data[1] = color->s;
}

void backlight_config_set_value( uint8_t *data )
{
	bool reinitialize = false;
	uint8_t *value_id = &(data[0]);
	uint8_t *value_data = &(data[1]);
	switch ( *value_id )
	{
#if defined (RGB_BACKLIGHT_ZEAL60) || defined(RGB_BACKLIGHT_ZEAL65)
		case id_use_split_backspace:
		{
			g_config.use_split_backspace = (bool)*value_data;
			reinitialize = true;
			break;
		}
#endif
#if defined (RGB_BACKLIGHT_ZEAL60)
		case id_use_split_left_shift:
		{
			g_config.use_split_left_shift = (bool)*value_data;
			reinitialize = true;
			break;
		}
		case id_use_split_right_shift:
		{
			g_config.use_split_right_shift = (bool)*value_data;
			reinitialize = true;
			break;
		}
		case id_use_7u_spacebar:
		{
			g_config.use_7u_spacebar = (bool)*value_data;
			reinitialize = true;
			break;
		}
		case id_use_iso_enter:
		{
			g_config.use_iso_enter = (bool)*value_data;
			reinitialize = true;
			break;
		}
		case id_disable_hhkb_blocker_leds:
		{
			g_config.disable_hhkb_blocker_leds = (bool)*value_data;
			reinitialize = true;
			break;
		}
#endif
		case id_disable_when_usb_suspended:
		{
			g_config.disable_when_usb_suspended = (bool)*value_data;
			break;
		}
		case id_disable_after_timeout:
		{
			g_config.disable_after_timeout = *value_data;
			break;
		}
		case id_brightness:
		{
			g_config.brightness = *value_data;
			break;
		}
		case id_effect:
		{
			g_config.effect = *value_data;
			break;
		}
		case id_effect_speed:
		{
			g_config.effect_speed = *value_data;
			break;
		}
		case id_color_1:
		{
			_set_color( &(g_config.color_1), value_data );
			break;
		}
		case id_color_2:
		{
			_set_color( &(g_config.color_2), value_data );
			break;
		}
		case id_caps_lock_indicator_color:
		{
			_set_color( &(g_config.caps_lock_indicator.color), value_data );
			break;
		}
		case id_caps_lock_indicator_row_col:
		{
			backlight_set_indicator_index( &(g_config.caps_lock_indicator.index), value_data[0], value_data[1] );
			break;
		}
		case id_layer_1_indicator_color:
		{
			_set_color( &(g_config.layer_1_indicator.color), value_data );
			break;
		}
		case id_layer_1_indicator_row_col:
		{
			backlight_set_indicator_index( &(g_config.layer_1_indicator.index), value_data[0], value_data[1] );
			break;
		}
		case id_layer_2_indicator_color:
		{
			_set_color( &(g_config.layer_2_indicator.color), value_data );
			break;
		}
		case id_layer_2_indicator_row_col:
		{
			backlight_set_indicator_index( &(g_config.layer_2_indicator.index), value_data[0], value_data[1] );
			break;
		}
		case id_layer_3_indicator_color:
		{
			_set_color( &(g_config.layer_3_indicator.color), value_data );
			break;
		}
		case id_layer_3_indicator_row_col:
		{
			backlight_set_indicator_index( &(g_config.layer_3_indicator.index), value_data[0], value_data[1] );
			break;
		}
		case id_alphas_mods:
		{
			for ( int i=0; i<5; i++ )
			{
				g_config.alphas_mods[i] = ( *(value_data+i*2) << 8 ) | ( *(value_data+i*2+1) );
			}
		}
#if defined(RGB_BACKLIGHT_M6_B)
		case id_custom_color:
		{
			uint8_t index = value_data[0];
			if ( index >= 0 && index <= 6 )
			{
				_set_color( &(g_config.custom_color[index]), &(value_data[1]) );
			}
		}
#endif
	}

	if ( reinitialize )
	{
		backlight_init_drivers();
	}
}

void backlight_config_get_value( uint8_t *data )
{
	uint8_t *value_id = &(data[0]);
	uint8_t *value_data = &(data[1]);
	switch ( *value_id )
	{
		case id_use_split_backspace:
		{
			*value_data = ( g_config.use_split_backspace ? 1 : 0 );
			break;
		}
		case id_use_split_left_shift:
		{
			*value_data = ( g_config.use_split_left_shift ? 1 : 0 );
			break;
		}
		case id_use_split_right_shift:
		{
			*value_data = ( g_config.use_split_right_shift ? 1 : 0 );
			break;
		}
		case id_use_7u_spacebar:
		{
			*value_data = ( g_config.use_7u_spacebar ? 1 : 0 );
			break;
		}
		case id_use_iso_enter:
		{
			*value_data = ( g_config.use_iso_enter ? 1 : 0 );
			break;
		}
		case id_disable_when_usb_suspended:
		{
			*value_data = ( g_config.disable_when_usb_suspended ? 1 : 0 );
			break;
		}
		case id_disable_hhkb_blocker_leds:
		{
			*value_data = ( g_config.disable_hhkb_blocker_leds ? 1 : 0 );
			break;
		}
		case id_disable_after_timeout:
		{
			*value_data = g_config.disable_after_timeout;
			break;
		}
		case id_brightness:
		{
			*value_data = g_config.brightness;
			break;
		}
		case id_effect:
		{
			*value_data = g_config.effect;
			break;
		}
		case id_effect_speed:
		{
			*value_data = g_config.effect_speed;
			break;
		}
		case id_color_1:
		{
			_get_color( &(g_config.color_1), value_data );
			break;
		}
		case id_color_2:
		{
			_get_color( &(g_config.color_2), value_data );
			break;
		}
		case id_caps_lock_indicator_color:
		{
			_get_color( &(g_config.caps_lock_indicator.color), value_data );
			break;
		}
		case id_caps_lock_indicator_row_col:
		{
			backlight_get_indicator_row_col( g_config.caps_lock_indicator.index, &(value_data[0]), &(value_data[1]) );
			break;
		}
		case id_layer_1_indicator_color:
		{
			_get_color( &(g_config.layer_1_indicator.color), value_data );
			break;
		}
		case id_layer_1_indicator_row_col:
		{
			backlight_get_indicator_row_col( g_config.layer_1_indicator.index, &(value_data[0]), &(value_data[1]) );
			break;
		}
		case id_layer_2_indicator_color:
		{
			_get_color( &(g_config.layer_2_indicator.color), value_data );
			break;
		}
		case id_layer_2_indicator_row_col:
		{
			backlight_get_indicator_row_col( g_config.layer_2_indicator.index, &(value_data[0]), &(value_data[1]) );
			break;
		}
		case id_layer_3_indicator_color:
		{
			_get_color( &(g_config.layer_3_indicator.color), value_data );
			break;
		}
		case id_layer_3_indicator_row_col:
		{
			backlight_get_indicator_row_col( g_config.layer_3_indicator.index, &(value_data[0]), &(value_data[1]) );
			break;
		}
		case id_alphas_mods:
		{
			for ( int i=0; i<5; i++ )
			{
				*(value_data+i*2) = g_config.alphas_mods[i] >> 8;
				*(value_data+i*2+1)	= g_config.alphas_mods[i] & 0xFF;
			}
		}
#if defined(RGB_BACKLIGHT_M6_B)
		case id_custom_color:
		{
			uint8_t index = value_data[0];
			if ( index >= 0 && index <= 6 )
			{
				_get_color( &(g_config.custom_color[index]), &(value_data[1]) );
			}
		}
#endif
	}
}

void backlight_config_set_alphas_mods( uint16_t *alphas_mods )
{
	for ( int i=0; i<5; i++ )
	{
		g_config.alphas_mods[i] = alphas_mods[i];
	}

	backlight_config_save();
}

void backlight_config_load(void)
{
	eeprom_read_block( &g_config, ((void*)RGB_BACKLIGHT_CONFIG_EEPROM_ADDR), sizeof(backlight_config) );
}

void backlight_config_save(void)
{
	eeprom_update_block( &g_config, ((void*)RGB_BACKLIGHT_CONFIG_EEPROM_ADDR), sizeof(backlight_config) );
}

void backlight_init_drivers(void)
{
	// Initialize I2C
	i2c_init();

#if defined(RGB_BACKLIGHT_M6_B)
	IS31FL3218_init();
#elif defined(RGB_BACKLIGHT_HS60)
	IS31FL3733_init( ISSI_ADDR_1 );

	for ( int index = 0; index < BACKLIGHT_LED_COUNT; index++ )
	{
#if defined (HS60_ANSI)
		bool enabled = !( ( index == 48-1 ) || //LA48
						  ( index == 51-1 ) || //LA51
						  ( index == 61-1 ) ); //LA61
#elif defined (HS60_HHKB)
		bool enabled = !( ( index == 61-1 ) || //LA61
						  ( index == 62-1 ) ); //LA62
#else //HS60_ISO
		bool enabled = !( ( index == 51-1 ) || //LA51
						  ( index == 61-1 ) ); //LA61
#endif
				// This only caches it for later
		IS31FL3733_set_led_control_register( index, enabled, enabled, enabled );
	}
	// This actually updates the LED drivers
	IS31FL3733_update_led_control_registers( ISSI_ADDR_1, ISSI_ADDR_2 );
#else
	IS31FL3731_init( ISSI_ADDR_1 );
	IS31FL3731_init( ISSI_ADDR_2 );

	for ( int index = 0; index < BACKLIGHT_LED_COUNT; index++ )
	{
		// OR the possible "disabled" cases together, then NOT the result to get the enabled state
		// LC6 LD13 not present on Zeal65
#if defined (RGB_BACKLIGHT_ZEAL65)
		bool enabled = !( ( index == 18+5 && !g_config.use_split_backspace ) || // LB5
						  ( index == 36+6 ) || // LC6
						  ( index == 54+13 ) ); // LD13
#elif defined (RGB_BACKLIGHT_KOYU)
		bool enabled = !( ( index == 36+15 ) || // LC15
						  ( index == 54+13 ) || // LD13
						  ( index == 54+17 ) ); // LD17
#elif defined (RGB_BACKLIGHT_M60_A)
		bool enabled = !(
		// LB6 LB7 LB8 LB15 LB16 LB17 not present on M60-A
						  ( index == 18+6 ) || // LB6
						  ( index == 18+7 ) || // LB7
						  ( index == 18+8 ) || // LB8
						  ( index == 18+15 ) || // LB15
						  ( index == 18+16 ) || // LB16
						  ( index == 18+17 ) || // LB17
		// HHKB blockers (LC17, LD17) and ISO extra keys (LC15,LD13) not present on M60-A
						  ( index == 36+17 ) || // LC17
						  ( index == 54+17 ) || // LD17
						  ( index == 36+15 ) || // LC15
						  ( index == 54+13 ) ); // LD13
#elif defined (RGB_BACKLIGHT_ZEAL60)
		// LB6 LB7 LB8 LB15 LB16 LB17 not present on Zeal60
		bool enabled = !( ( index == 18+5 && !g_config.use_split_backspace ) || // LB5
						  ( index == 36+15 && !g_config.use_split_left_shift ) || // LC15
						  ( index == 54+8 && !g_config.use_split_right_shift ) || // LD8
						  ( index == 54+13 && g_config.use_7u_spacebar ) || // LD13
						  ( index == 36+17 && g_config.disable_hhkb_blocker_leds ) || // LC17
						  ( index == 54+17 && g_config.disable_hhkb_blocker_leds ) ||  // LD17
						  ( index == 18+6 ) || // LB6
						  ( index == 18+7 ) || // LB7
						  ( index == 18+8 ) || // LB8
						  ( index == 18+15 ) || // LB15
						  ( index == 18+16 ) || // LB16
						  ( index == 18+17 ) ); // LB17
#endif
		// This only caches it for later
		IS31FL3731_set_led_control_register( index, enabled, enabled, enabled );
	}
	// This actually updates the LED drivers
	IS31FL3731_update_led_control_registers( ISSI_ADDR_1, ISSI_ADDR_2 );
#endif // !defined(RGB_BACKLIGHT_M6_B)

	// TODO: put the 1 second startup delay here?

	// clear the key hits
	for ( int led=0; led<BACKLIGHT_LED_COUNT; led++ )
	{
		g_key_hit[led] = 255;
	}
}

bool process_record_backlight(uint16_t keycode, keyrecord_t *record)
{
	// Record keypresses for backlight effects
	if ( record->event.pressed )
	{
		backlight_set_key_hit( record->event.key.row, record->event.key.col );
	}

	switch(keycode)
	{
		case BR_OFF:
			if (record->event.pressed)
			{
				g_config.effect = 0;
				backlight_config_save();
			}
			return false;
			break;
		case BR_MAX:
			if (record->event.pressed)
			{
				g_config.brightness = 255;
				backlight_config_save();
			}
			return false;
			break;
		case BR_INC:
			if (record->event.pressed)
			{
				backlight_brightness_increase();
			}
			return false;
			break;
		case BR_DEC:
			if (record->event.pressed)
			{
				backlight_brightness_decrease();
			}
			return false;
			break;
		case EF_INC:
			if (record->event.pressed)
			{
				backlight_effect_increase();
			}
			return false;
			break;
		case EF_DEC:
			if (record->event.pressed)
			{
				backlight_effect_decrease();
			}
			return false;
			break;
		case ES_INC:
			if (record->event.pressed)
			{
				backlight_effect_speed_increase();
			}
			return false;
			break;
		case ES_DEC:
			if (record->event.pressed)
			{
				backlight_effect_speed_decrease();
			}
			return false;
			break;
		case H1_INC:
			if (record->event.pressed)
			{
				backlight_color_1_hue_increase();
			}
			return false;
			break;
		case H1_DEC:
			if (record->event.pressed)
			{
				backlight_color_1_hue_decrease();
			}
			return false;
			break;
		case S1_INC:
			if (record->event.pressed)
			{
				backlight_color_1_sat_increase();
			}
			return false;
			break;
		case S1_DEC:
			if (record->event.pressed)
			{
				backlight_color_1_sat_decrease();
				break;
			}
			return false;
			break;
		case H2_INC:
			if (record->event.pressed)
			{
				backlight_color_2_hue_increase();
			}
			return false;
			break;
		case H2_DEC:
			if (record->event.pressed)
			{
				backlight_color_2_hue_decrease();
			}
			return false;
			break;
		case S2_INC:
			if (record->event.pressed)
			{
				backlight_color_2_sat_increase();
			}
			return false;
			break;
		case S2_DEC:
			if (record->event.pressed)
			{
				backlight_color_2_sat_decrease();
				break;
			}
			return false;
			break;
	}

	return true;
}

// Deals with the messy details of incrementing an integer
uint8_t increment( uint8_t value, uint8_t step, uint8_t min, uint8_t max )
{
	int16_t new_value = value;
	new_value += step;
	if (new_value <= max)
	{
		return new_value;
	} else {
		return new_value - max - 1; 
	}
//	return MIN( MAX( new_value, min ), max );
}

uint8_t decrement( uint8_t value, uint8_t step, uint8_t min, uint8_t max )
{
	int16_t new_value = value;
	new_value -= step;
	return MIN( MAX( new_value, min ), max );
}

void backlight_effect_increase(void)
{
	g_config.effect = increment( g_config.effect, 1, 0, BACKLIGHT_EFFECT_MAX );
	backlight_config_save();
}

void backlight_effect_decrease(void)
{
	g_config.effect = decrement( g_config.effect, 1, 0, BACKLIGHT_EFFECT_MAX );
	backlight_config_save();
}

void backlight_effect_speed_increase(void)
{
	g_config.effect_speed = increment( g_config.effect_speed, 1, 0, 3 );
	backlight_config_save();
}

void backlight_effect_speed_decrease(void)
{
	g_config.effect_speed = decrement( g_config.effect_speed, 1, 0, 3 );
	backlight_config_save();
}

void backlight_brightness_increase(void)
{
	g_config.brightness = increment( g_config.brightness, 8, 0, 255 );
	backlight_config_save();
}

void backlight_brightness_decrease(void)
{
	g_config.brightness = decrement( g_config.brightness, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_1_hue_increase(void)
{
	g_config.color_1.h = increment( g_config.color_1.h, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_1_hue_decrease(void)
{
	g_config.color_1.h = decrement( g_config.color_1.h, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_1_sat_increase(void)
{
	g_config.color_1.s = increment( g_config.color_1.s, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_1_sat_decrease(void)
{
	g_config.color_1.s = decrement( g_config.color_1.s, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_2_hue_increase(void)
{
	g_config.color_2.h = increment( g_config.color_2.h, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_2_hue_decrease(void)
{
	g_config.color_2.h = decrement( g_config.color_2.h, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_2_sat_increase(void)
{
	g_config.color_2.s = increment( g_config.color_2.s, 8, 0, 255 );
	backlight_config_save();
}

void backlight_color_2_sat_decrease(void)
{
	g_config.color_2.s = decrement( g_config.color_2.s, 8, 0, 255 );
	backlight_config_save();
}

#if defined(RGB_DEBUGGING_ONLY)
void backlight_test_led( uint8_t index, bool red, bool green, bool blue )
{
	for ( int i=0; i<BACKLIGHT_LED_COUNT; i++ )
	{
		if ( i == index )
		{
			IS31FL3731_set_led_control_register( i, red, green, blue );
		}
		else
		{
			IS31FL3731_set_led_control_register( i, false, false, false );
		}
	}
}

void backlight_debug_led( bool state )
{
	if (state)
	{
		// Output high.
		DDRE |= (1<<6);
		PORTE |= (1<<6);
	}
	else
	{
		// Output low.
		DDRE &= ~(1<<6);
		PORTE &= ~(1<<6);
	}
}
#endif // defined(RGB_DEBUGGING_ONLY)

#endif // BACKLIGHT_ENABLED
