

#include "ui.h"

static lv_obj_t *gauge;
static lv_timer_t *gauge_timer;

void gauge_timer_cb(lv_timer_t *timer)
{
    static int value = 0;
    value = (value + 2) % 101;   // 0–100
    lv_meter_set_indicator_value(gauge, timer->user_data, value);
}

lv_meter_indicator_t *needle_left;
lv_meter_indicator_t *needle_right;
lv_meter_indicator_t *needle_upper_outline;
lv_meter_indicator_t *needle_lower_outline;

lv_obj_t *label_stft_value;
lv_obj_t *label_ltft_value;
lv_obj_t *label_stft_text;
lv_obj_t *label_ltft_text;
lv_obj_t *label_stft_percent;
lv_obj_t *label_ltft_percent;
lv_obj_t *tri_needle_upper;
lv_obj_t *tri_needle_lower;
lv_obj_t *needle_tip_upper;
lv_obj_t *needle_tip_lower;

void create_dual_gauge(void)
{
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                              true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    // ===== SCALES =====
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

    gauge = lv_meter_create(lv_scr_act());
    lv_obj_set_style_bg_color(gauge, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(gauge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(gauge);
    lv_obj_set_size(gauge, 466, 466);
    lv_obj_set_style_pad_all(gauge, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(gauge, 0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(gauge, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(gauge, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(gauge, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(gauge, 0, LV_PART_MAIN);


    // ===== SCALES =====
    lv_meter_scale_t *scale_upper = lv_meter_add_scale(gauge);
    lv_meter_scale_t *scale_lower = lv_meter_add_scale(gauge);

    // ===== COLORS (dark / faded OEM style) =====
    lv_color_t COL_GREY  = lv_color_make(110, 110, 110);
    lv_color_t COL_GREEN = lv_color_make(40, 130, 90);
    lv_color_t COL_TICK  = lv_color_make(200, 200, 255);
    
    // 150° arcs (changed from 160°)
    // Upper: centered at top (195° start angle)
    lv_meter_set_scale_range(gauge, scale_upper, 0, 300, 150, 195);
    
    // Lower: centered at bottom (15° start angle)
    lv_meter_set_scale_range(gauge, scale_lower, 0, 300, 150, 15);

    // Remove default ticks
    //lv_meter_set_scale_ticks(gauge, scale_upper, 0, 0, 0, lv_color_black());
    //lv_meter_set_scale_ticks(gauge, scale_lower, 0, 0, 0, lv_color_black());

    // ===== ARC THICKNESS =====
    int arc_thick   = 5;   // grey arc thickness
    int green_thick = 8;   // green overlay thickness

    // ===== GREY BASE ARCS =====
    // lv_meter_indicator_t *u_arc = lv_meter_add_arc(gauge, scale_upper, arc_thick, COL_GREY, 0);
    // lv_meter_set_indicator_start_value(gauge, u_arc, 0);
    // lv_meter_set_indicator_end_value(gauge, u_arc, 30);

    // Configure upper scale - 174 degrees with 3 line ticks (edges + center) 
    lv_meter_set_scale_ticks(gauge, scale_upper, 2, 2, 12, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_ticks(gauge, scale_lower, 2, 2, 12, lv_palette_main(LV_PALETTE_GREY));
    //lv_meter_set_scale_major_ticks(gauge, scale_upper, 0, 0, 0, lv_color_black(), 0); 
    //lv_meter_set_scale_range(gauge, scale_upper, 0, 30, 174, 183); 
    // 174 degrees, starting at 183° (upper arc)

    lv_meter_indicator_t *l_arc = lv_meter_add_arc(gauge, scale_lower, arc_thick, COL_GREY, 0);
    lv_meter_set_indicator_start_value(gauge, l_arc, 0);
    lv_meter_set_indicator_end_value(gauge, l_arc, 300);

    lv_meter_indicator_t *u_arc = lv_meter_add_arc(gauge, scale_upper, arc_thick, COL_GREY, 0);
    lv_meter_set_indicator_start_value(gauge, u_arc, 0);
    lv_meter_set_indicator_end_value(gauge, u_arc, 300);

    // ===== GREEN MIDDLE SEGMENTS =====
    // middle third + tiny gap between upper & lower segments
    int g_start = 110;
    int g_end   = 190;

    lv_meter_indicator_t *u_green = lv_meter_add_arc(gauge, scale_upper, green_thick, lv_palette_main(LV_PALETTE_GREEN), -5);
    lv_meter_set_indicator_start_value(gauge, u_green, g_start);
    lv_meter_set_indicator_end_value(gauge, u_green, g_end);

    lv_meter_indicator_t *l_green = lv_meter_add_arc(gauge, scale_lower, green_thick, lv_palette_main(LV_PALETTE_GREEN), -5);
    lv_meter_set_indicator_start_value(gauge, l_green, g_start);
    lv_meter_set_indicator_end_value(gauge, l_green, g_end);

    // ===== TICK MARKS AT ARC ENDS =====
    // Create thin arc segments at the ends to act as tick marks
    int tick_width = 3;
    int tick_radius_mod = -2;  // Slightly inside the arc
    
    // Upper scale - left tick (at 0)
    lv_meter_indicator_t *u_tick_left = lv_meter_add_arc(gauge, scale_upper, tick_width, COL_TICK, tick_radius_mod);
    lv_meter_set_indicator_start_value(gauge, u_tick_left, 20);
    lv_meter_set_indicator_end_value(gauge, u_tick_left, 20.5);
    
    // // Upper scale - right tick (at 30)
    // lv_meter_indicator_t *u_tick_right = lv_meter_add_arc(gauge, scale_upper, tick_width, COL_TICK, tick_radius_mod);
    // lv_meter_set_indicator_start_value(gauge, u_tick_right, 29.5);
    // lv_meter_set_indicator_end_value(gauge, u_tick_right, 30);
    
    // // Lower scale - left tick (at 0)
    // lv_meter_indicator_t *l_tick_left = lv_meter_add_arc(gauge, scale_lower, tick_width, COL_TICK, tick_radius_mod);
    // lv_meter_set_indicator_start_value(gauge, l_tick_left, 0);
    // lv_meter_set_indicator_end_value(gauge, l_tick_left, 0.5);
    
    // // Lower scale - right tick (at 30)
    // lv_meter_indicator_t *l_tick_right = lv_meter_add_arc(gauge, scale_lower, tick_width, COL_TICK, tick_radius_mod);
    // lv_meter_set_indicator_start_value(gauge, l_tick_right, 29.5);
    // lv_meter_set_indicator_end_value(gauge, l_tick_right, 30);

    // ===== NEEDLES WITH BLACK BEVEL =====
    lv_color_t needle_color = lv_color_make(200, 220, 255);
    int needle_thick = 4;

    // black outline (behind)
    needle_upper_outline = lv_meter_add_needle_line(gauge, scale_upper, needle_thick + 2, lv_color_black(), 0);
    needle_lower_outline = lv_meter_add_needle_line(gauge, scale_lower, needle_thick + 2, lv_color_black(), 0);

    // white needles
    needle_left  = lv_meter_add_needle_line(gauge, scale_upper, needle_thick, needle_color, 0);
    needle_right = lv_meter_add_needle_line(gauge, scale_lower, needle_thick, needle_color, 0);

    // lv_meter_set_indicator_value(gauge, needle_left, 10);
    // lv_meter_set_indicator_value(gauge, needle_right, 14);
    // lv_meter_set_indicator_value(gauge, needle_upper_outline, 10);
    // lv_meter_set_indicator_value(gauge, needle_lower_outline, 14);

    // ===== SMALL CENTER CIRCLE =====
    lv_obj_t *center_circle = lv_obj_create(gauge);
    lv_obj_set_size(center_circle, 140, 140);
    lv_obj_center(center_circle);
    lv_obj_set_style_bg_color(center_circle, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(center_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(center_circle, 0, 0);
    lv_obj_set_style_radius(center_circle, LV_RADIUS_CIRCLE, 0);

    // ===== LABELS =====
    // STFT
    lv_obj_t *label_stft = lv_label_create(gauge);
    lv_label_set_text(label_stft, "STFT");
    lv_obj_align(label_stft, LV_ALIGN_CENTER, 0, -50);
    lv_obj_set_style_text_color(label_stft, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(label_stft, &lv_font_montserrat_14, 0);
    

    label_stft_value = lv_label_create(gauge);
    lv_label_set_text(label_stft_value, "10.0");
    lv_obj_align(label_stft_value, LV_ALIGN_CENTER, 0, -25);
    lv_obj_set_style_text_color(label_stft_value, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(label_stft_value, &lv_font_montserrat_48, 0);
    lv_obj_set_width(label_stft_value, 150);   // Set fixed width
    lv_label_set_long_mode(label_stft_value, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_stft_value, LV_TEXT_ALIGN_CENTER, 0);

    // LTFT
    lv_obj_t *label_ltft = lv_label_create(gauge);
    lv_label_set_text(label_ltft, "LTFT");
    lv_obj_align(label_ltft, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_text_color(label_ltft, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(label_ltft, &lv_font_montserrat_14, 0);

    label_ltft_value = lv_label_create(gauge);
    lv_label_set_text(label_ltft_value, "14.0");
    lv_obj_align(label_ltft_value, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_style_text_color(label_ltft_value, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(label_ltft_value, &lv_font_montserrat_48, 0);
    lv_obj_set_width(label_ltft_value, 150);   // Set fixed width
    lv_label_set_long_mode(label_ltft_value, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label_ltft_value, LV_TEXT_ALIGN_CENTER, 0);
}




/**
 * Update STFT (upper) gauge value and label
 * @param value STFT value (0-30 range, can use negative by offsetting) 
 */
void update_stft_gauge(float value)
{
    // Clamp value to valid range
    if (value < 0) value = 0;
    if (value > 30) value = 30;
    
    // Update needle positions
    lv_meter_set_indicator_value(gauge, needle_left, (int32_t)(value * 10));
    lv_meter_set_indicator_value(gauge, needle_upper_outline, (int32_t)(value * 10));
    
    // Update label
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", value);
    lv_label_set_text(label_stft_value, buf);
}


void update_stft_gauge_string(char buf[16])
{

    lv_label_set_text(label_stft_value, buf);
}

/**
 * Update LTFT (lower) gauge value and label
 * @param value LTFT value (0-30 range)
 */
void update_ltft_gauge(float value)
{
    // Clamp value to valid range
    if (value < 0) value = 0;
    if (value > 300) value = 300;
    
    // Update needle positions
    lv_meter_set_indicator_value(gauge, needle_right, 300 - (int32_t)(value * 10));
    lv_meter_set_indicator_value(gauge, needle_lower_outline, 300 - (int32_t)(value * 10));
    
    // Update label
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", value);
    lv_label_set_text(label_ltft_value, buf);
}

/**
 * Update both gauges at once
 * @param stft_value STFT value (0-30)
 * @param ltft_value LTFT value (0-30)
 */
void update_both_gauges(float stft_value, float ltft_value)
{
    update_stft_gauge(stft_value);
    update_ltft_gauge(ltft_value);
}



