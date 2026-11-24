// Copyright 2026 Zen75 community firmware contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Extra RGB matrix effects for the Zen75 (fk2120).
//
// These are appended after the stock QMK effects, so every stock effect keeps
// its number (DIGITAL_RAIN, the "matrix" effect, is untouched). All of the
// effects below follow the hue / saturation / brightness / speed settings, so
// they can be recoloured with the RGB_HUI / RGB_SAI keys or the VIA colour
// picker.

RGB_MATRIX_EFFECT(NASA_WORM)
#if defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS)
RGB_MATRIX_EFFECT(DIGITAL_RAIN_HUE)
RGB_MATRIX_EFFECT(FIRE)
#endif
RGB_MATRIX_EFFECT(AURORA)
RGB_MATRIX_EFFECT(SCANNER)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

// Physical row (0 = F-row ... 5 = spacebar row) of an LED, from its y position.
static uint8_t zen_led_row(uint8_t i) {
    uint8_t row = g_led_config.point[i].y / 10; // y is 10, 20 ... 60
    row         = row ? row - 1 : 0;
    return row > 5 ? 5 : row;
}

static void zen_set_hsv(uint8_t i, uint8_t h, uint8_t s, uint8_t v) {
    if (v == 0) {
        rgb_matrix_set_color(i, 0, 0, 0);
        return;
    }
    HSV hsv = {h, s, v};
    RGB rgb = rgb_matrix_hsv_to_rgb(hsv);
    rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
}

// ---------------------------------------------------------------------------
// NASA_WORM: the "NASA" worm logotype scrolling slowly from right to left.
//
// The lettering is a 5-row bitmap drawn on the top five key rows (the bottom
// row has too few keys to carry a letter stroke). It is sampled at each LED's
// physical x position, so the row stagger does not shear the letters, and
// blended between bitmap columns so the scroll is smooth at low speed.
//
//   ##..# ..#.. .#### ..#..
//   #.#.# .#.#. #.... .#.#.
//   #.#.# .#.#. .###. .#.#.
//   #.#.# #...# ....# #...#
//   #..## #...# ####. #...#
//
// One byte per bitmap column, bit n = row n. 23 columns of lettering plus 9
// blank columns: the 32 column period keeps the wrap-around seamless.
// ---------------------------------------------------------------------------
#    define NASA_WORM_COLUMNS 32 // must be a power of two
static const uint8_t PROGMEM nasa_worm_bitmap[NASA_WORM_COLUMNS] = {
    0x1f, 0x01, 0x0e, 0x10, 0x1f, 0x00, // N
    0x18, 0x06, 0x01, 0x06, 0x18, 0x00, // A
    0x12, 0x15, 0x15, 0x15, 0x09, 0x00, // S
    0x18, 0x06, 0x01, 0x06, 0x18, 0x00, // A
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

bool NASA_WORM(effect_params_t* params) {
    // Scroll position in 1/1024 of 1/256 of a bitmap column.
    static uint32_t scroll     = 0;
    static uint32_t last_timer = 0;

    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    if (params->init) {
        // Start in the blank run, with the N about to enter from the right.
        scroll     = (uint32_t)(NASA_WORM_COLUMNS - 9) << 18;
        last_timer = g_rgb_timer;
    }
    if (params->iter == 0) {
        uint32_t elapsed = g_rgb_timer - last_timer;
        last_timer       = g_rgb_timer;
        if (elapsed > 100) elapsed = 100;
        // About 1.1 columns/s at full speed down to 0.12 columns/s at speed 0.
        scroll += elapsed * ((uint32_t)rgb_matrix_config.speed + 32);
    }

    const uint16_t offset = (uint16_t)(scroll >> 10);

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        const uint8_t row = zen_led_row(i);
        uint8_t       lit = 0;
        if (row < 5) {
            // Keys are 15 units apart: x * 17 is x in 1/256 columns.
            const uint16_t u    = (uint16_t)g_led_config.point[i].x * 17 + offset;
            const uint8_t  col  = (u >> 8) & (NASA_WORM_COLUMNS - 1);
            const uint8_t  frac = ease8InOutCubic(u & 0xff);
            const uint8_t  a    = (pgm_read_byte(&nasa_worm_bitmap[col]) >> row) & 1;
            const uint8_t  b    = (pgm_read_byte(&nasa_worm_bitmap[(col + 1) & (NASA_WORM_COLUMNS - 1)]) >> row) & 1;
            lit                 = (a ? 255 - frac : 0) + (b ? frac : 0);
        }
        zen_set_hsv(i, rgb_matrix_config.hsv.h, rgb_matrix_config.hsv.s, scale8(rgb_matrix_config.hsv.v, lit));
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#    if defined(RGB_MATRIX_FRAMEBUFFER_EFFECTS)
// ---------------------------------------------------------------------------
// DIGITAL_RAIN_HUE: the same falling-code animation as the stock DIGITAL_RAIN,
// but drawn in the selected colour instead of fixed green. The stock effect is
// left exactly as it was.
// ---------------------------------------------------------------------------
#        ifndef RGB_DIGITAL_RAIN_HUE_DROPS
#            ifdef RGB_DIGITAL_RAIN_DROPS
#                define RGB_DIGITAL_RAIN_HUE_DROPS RGB_DIGITAL_RAIN_DROPS
#            else
#                define RGB_DIGITAL_RAIN_HUE_DROPS 24
#            endif
#        endif

bool DIGITAL_RAIN_HUE(effect_params_t* params) {
    const uint8_t drop_ticks    = 28;
    const uint8_t max_intensity = rgb_matrix_config.hsv.v;
    const uint8_t tail_top      = (((uint16_t)max_intensity) * 3) >> 2;

    static uint8_t drop  = 0;
    static uint8_t decay = 0;

    if (params->init) {
        rgb_matrix_set_color_all(0, 0, 0);
        memset(g_rgb_frame_buffer, 0, sizeof(g_rgb_frame_buffer));
        drop = 0;
    }
    if (max_intensity < 4) {
        // Too dim to animate (and it would divide by zero below).
        rgb_matrix_set_color_all(0, 0, 0);
        return false;
    }
    const uint8_t decay_ticks = 0xff / max_intensity;

    decay++;
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            uint8_t* cell = &g_rgb_frame_buffer[row][col];
            if (row == 0 && drop == 0 && rand() < RAND_MAX / RGB_DIGITAL_RAIN_HUE_DROPS) {
                *cell = max_intensity;
            } else if (*cell > 0 && *cell < max_intensity) {
                if (decay >= decay_ticks) {
                    (*cell)--;
                }
            }

            uint8_t led[LED_HITS_TO_REMEMBER];
            if (rgb_matrix_map_row_column_to_led(row, col, led) > 0) {
                const uint8_t level = *cell > max_intensity ? max_intensity : *cell;
                if (level > tail_top) {
                    // Head of the drop: full brightness, washed out towards white.
                    const uint8_t whiten = (uint8_t)((uint16_t)191 * (level - tail_top) / (max_intensity - tail_top));
                    zen_set_hsv(led[0], rgb_matrix_config.hsv.h, scale8(rgb_matrix_config.hsv.s, 255 - whiten), max_intensity);
                } else {
                    // Tail: the selected colour, fading out.
                    zen_set_hsv(led[0], rgb_matrix_config.hsv.h, rgb_matrix_config.hsv.s, (uint8_t)((uint16_t)max_intensity * level / tail_top));
                }
            }
        }
    }
    if (decay >= decay_ticks) {
        decay = 0;
    }

    if (++drop > drop_ticks) {
        drop = 0;
        for (uint8_t row = MATRIX_ROWS - 1; row > 0; row--) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                // Let a bright pixel on the bottom row start to decay.
                if (row == MATRIX_ROWS - 1 && g_rgb_frame_buffer[row][col] >= max_intensity) {
                    g_rgb_frame_buffer[row][col] = max_intensity - 1;
                }
                // Move the head of each drop down one row.
                if (g_rgb_frame_buffer[row - 1][col] >= max_intensity) {
                    g_rgb_frame_buffer[row - 1][col] = max_intensity - 1;
                    g_rgb_frame_buffer[row][col]     = max_intensity;
                }
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// FIRE: flames rising from the bottom row. The selected hue is the colour of
// the embers; hotter cells shift 40 hue steps onwards (red -> yellow at hue 0,
// blue -> violet at hue 170, ...) and lose saturation.
// ---------------------------------------------------------------------------
#        define ZEN_FIRE_COLS 15 // 225 units of width / 15 units per key

static void zen_fire_step(void) {
    // Carry heat upwards, cooling as it goes.
    for (uint8_t row = 0; row < 5; row++) {
        for (uint8_t col = 0; col < ZEN_FIRE_COLS; col++) {
            const uint8_t  left  = col ? col - 1 : 0;
            const uint8_t  right = col < ZEN_FIRE_COLS - 1 ? col + 1 : col;
            const uint16_t below = (uint16_t)g_rgb_frame_buffer[row + 1][col] * 2 + g_rgb_frame_buffer[row + 1][left] + g_rgb_frame_buffer[row + 1][right];
            g_rgb_frame_buffer[row][col] = qsub8(below / 4, random8_min_max(8, 56));
        }
    }
    // Stoke the bottom row.
    for (uint8_t col = 0; col < ZEN_FIRE_COLS; col++) {
        g_rgb_frame_buffer[5][col] = qadd8(scale8(g_rgb_frame_buffer[5][col], 150), random8_min_max(40, 150));
    }
}

bool FIRE(effect_params_t* params) {
    static uint32_t last_step = 0;

    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    if (params->init) {
        memset(g_rgb_frame_buffer, 0, sizeof(g_rgb_frame_buffer));
        last_step = g_rgb_timer;
    }
    if (params->iter == 0) {
        // One simulation step every 40 ms (full speed) to 130 ms (speed 0).
        const uint8_t interval = 130 - scale8(rgb_matrix_config.speed, 90);
        if (g_rgb_timer - last_step >= interval) {
            last_step = g_rgb_timer;
            zen_fire_step();
        }
    }

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        uint8_t col = g_led_config.point[i].x / 15;
        if (col >= ZEN_FIRE_COLS) col = ZEN_FIRE_COLS - 1;
        const uint8_t heat = g_rgb_frame_buffer[zen_led_row(i)][col];
        const uint8_t hue  = rgb_matrix_config.hsv.h + scale8(heat, 40);
        const uint8_t sat  = scale8(rgb_matrix_config.hsv.s, 255 - scale8(qsub8(heat, 160), 200));
        zen_set_hsv(i, hue, sat, scale8(rgb_matrix_config.hsv.v, qadd8(heat, heat)));
    }
    return rgb_matrix_check_finished_leds(led_max);
}
#    endif // RGB_MATRIX_FRAMEBUFFER_EFFECTS

// ---------------------------------------------------------------------------
// AURORA: slow curtains of light that drift about the selected hue.
// ---------------------------------------------------------------------------
bool AURORA(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    // Truncating to 8 bits keeps these seamless when g_rgb_timer wraps.
    const uint8_t t1 = scale16by8(g_rgb_timer, rgb_matrix_config.speed / 16 + 2);
    const uint8_t t2 = scale16by8(g_rgb_timer, rgb_matrix_config.speed / 24 + 1);

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        const uint8_t x     = g_led_config.point[i].x;
        const uint8_t y     = g_led_config.point[i].y;
        const uint8_t drift = avg8(sin8(x + t1), sin8((uint8_t)(y * 3 + x / 2) - t2));
        const uint8_t hue   = rgb_matrix_config.hsv.h + scale8(drift, 72) - 36;
        const uint8_t glow  = 80 + scale8(sin8((uint8_t)(x * 2 + y) - t1), 175);
        zen_set_hsv(i, hue, rgb_matrix_config.hsv.s, scale8(rgb_matrix_config.hsv.v, glow));
    }
    return rgb_matrix_check_finished_leds(led_max);
}

// ---------------------------------------------------------------------------
// SCANNER: a bright bar sweeping from side to side over a dim floor.
// ---------------------------------------------------------------------------
bool SCANNER(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    const uint8_t time  = scale16by8(g_rgb_timer, rgb_matrix_config.speed / 8 + 4);
    const uint8_t pos   = scale8(ease8InOutQuad(triwave8(time)), 224);
    const uint8_t floor = rgb_matrix_config.hsv.v / 10;

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        const uint8_t x    = g_led_config.point[i].x;
        const uint8_t dist = x > pos ? x - pos : pos - x;
        uint8_t       glow = dist < 42 ? 255 - dist * 6 : 0;
        glow               = scale8(glow, glow);
        zen_set_hsv(i, rgb_matrix_config.hsv.h, rgb_matrix_config.hsv.s, floor + scale8(rgb_matrix_config.hsv.v - floor, glow));
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
