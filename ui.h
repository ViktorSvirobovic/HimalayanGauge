

#ifndef _SQUARELINE_PROJECT_UI_H
#define _SQUARELINE_PROJECT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_helpers.h"


LV_FONT_DECLARE(ui_font_FontBottom);
LV_FONT_DECLARE(ui_font_FontTop);


void create_dual_gauge(void);
void update_both_gauges(float stft_value, float ltft_value);
void update_ltft_gauge(float value);
void update_stft_gauge(float value);
void update_stft_gauge_string(char buf[16]);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
