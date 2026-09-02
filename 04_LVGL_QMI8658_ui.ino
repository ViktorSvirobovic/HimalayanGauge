#include <Arduino.h>
#include <lvgl.h>
#include <stdlib.h>
#include <string.h>
#include <Wire.h>

#include "Arduino_GFX_Library.h"
#include "fonts/font_awesome_icons_small.h"
#include "lv_conf.h"
#include "pin_config.h"
#include "splash_0.h"

// ===== CONSTANTS =====
#define EXAMPLE_LVGL_TICK_PERIOD_MS 2
#define BATTERY_SYMBOL "\xEF\x97\x9F"
#define WATER_SYMBOL "\xEF\x98\x94"
#define TEST_MODE 1
#define NEEDLE_DT 0.016f
#define DEBUG_LOG 1

#define RXD1 43
#define TXD1 44

#if DEBUG_LOG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// ===== STRUCTURES =====
struct GaugeData {
  int horz_pos;
  int vert_pos;
  int min;
  int max;
};

struct MeterConfig {
  lv_obj_t **meter;
  lv_meter_indicator_t **indicator;
  int scale_min;
  int scale_max;
  int angle_range;
  int rotation;
};

// ===== FORWARD DECLARATIONS =====
void make_left_meter(void);
void make_right_meter(void);
void make_meter(const MeterConfig *config);
void make_styles(void);
void make_inner_circle(void);
void make_icons(void);
void make_daily_screen(void);
void create_dual_gauge(void);
void create_green_masks(void);
void update_water_temp(int new_water_temp);
void update_battery_voltage(float new_val);
void update_dual_gauge(float left_val, float right_val);
void battery_needle_tick(void);
void water_needle_tick(void);
void updateTestMode(void);
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void example_increase_lvgl_tick(void *arg);
void show_splash(void);
void switch_screen_cb(lv_timer_t *t);
void transition_to_main_ui(void);
void handle_elm_init(void);
void handle_obd_poll(void);
void sendELM(const char *cmd);
void sendOBD(const char *pid);
void parseOBDResponse(const char *resp);

// ===== GLOBAL DATA =====
extern const lv_img_dsc_t splash_0;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LCD_WIDTH * 40];
static lv_color_t buf2[LCD_WIDTH * 40];

GaugeData WaterData = {165, 0, 50, 130};
GaugeData BatteryData = {-165, 0, 10, 16};

static float current_battery_value = 0.0f;
static float current_water_value = 0.0f;
static int target_water_value = 0;
static float target_battery_value = 0.0f;
static float battery_velocity = 0.0f;
static float water_velocity = 0.0f;

bool ui_ready = false;

unsigned long lastRequest = 0;
const unsigned long requestInterval = 500;
float voltage = 0.0f;
float temperature = 0.0f;

float testVoltage = BatteryData.min;
float testTemperature = WaterData.min;
bool voltageIncreasing = true;
bool temperatureIncreasing = true;
unsigned long lastTestUpdate = 0;
const unsigned long testUpdateInterval = 100;
const float voltageStep = 0.1f;
const float temperatureStep = 0.2f;

lv_obj_t *splash_screen = NULL;
lv_obj_t *splash_img = NULL;
lv_obj_t *startup_scr = NULL;
lv_obj_t *daily_scr = NULL;
lv_obj_t *oil_meter = NULL;
lv_obj_t *water_meter = NULL;
lv_obj_t *mask_160 = NULL;
lv_obj_t *mask_100 = NULL;
lv_obj_t *battery_icon = NULL;
lv_obj_t *water_icon = NULL;
lv_meter_indicator_t *oil_indic = NULL;
lv_meter_indicator_t *water_indic = NULL;

static lv_style_t style_icon;

lv_color_t PALETTE_BLACK = LV_COLOR_MAKE(0, 0, 0);
lv_color_t PALETTE_WHITE = LV_COLOR_MAKE(255, 255, 255);
lv_color_t PALETTE_GREY = LV_COLOR_MAKE(90, 90, 90);

int TICK_WIDTH = 2;
int TICK_LENGTH = 15;
int TICK_TEXT_OFFSET = 30;
int OUTLINE_WIDTH = 2;
int NEEDLE_WIDTH = 5;
int NEEDLE_OFFSET = -10;
lv_color_t NEEDLE_COLOR = LV_COLOR_MAKE(255, 0, 0);
int HALF_METER_TICKS = 5;
const int BATTERY_SCALE = 10;
const int SPLASH_FADE_MS = 300;
const int SPLASH_MIN_MS = 3000;
const int OBD_REQUEST_GAP_MS = 50;
const int OBD_BUFFER_SIZE = 96;

char obdBuffer[OBD_BUFFER_SIZE];
int obdBufferPos = 0;
bool transition_started = false;
bool obd_voltage_pending = false;
unsigned long obd_voltage_due = 0;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1,
  LCD_SDIO2, LCD_SDIO3
);

Arduino_CO5300 *gfx = new Arduino_CO5300(
  bus, LCD_RESET, 0, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0
);

const char *elmInitCmds[] = {"ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP0"};
const int INIT_COUNT = sizeof(elmInitCmds) / sizeof(elmInitCmds[0]);

enum BootState {
  BOOT_SPLASH,
  BOOT_INIT,
  BOOT_READY,
  BOOT_DONE
};

BootState boot_state = BOOT_SPLASH;
unsigned long splash_start_time = 0;
int elm_step = 0;

void make_meter(const MeterConfig *config) {
  lv_obj_t *meter = lv_meter_create(daily_scr);
  *config->meter = meter;

  lv_obj_set_size(meter, 466, 466);
  lv_obj_set_style_bg_opa(meter, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(meter, 0, 0);
  lv_obj_center(meter);
  lv_obj_set_style_text_font(meter, &lv_font_montserrat_24, LV_PART_TICKS);
  lv_obj_set_style_text_color(meter, PALETTE_WHITE, LV_PART_TICKS);

  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_ticks(meter, scale, HALF_METER_TICKS, TICK_WIDTH, TICK_LENGTH, PALETTE_WHITE);
  lv_meter_set_scale_major_ticks(meter, scale, HALF_METER_TICKS - 1, TICK_WIDTH, TICK_LENGTH, PALETTE_WHITE, TICK_TEXT_OFFSET);
  lv_meter_set_scale_range(meter, scale, config->scale_min, config->scale_max, config->angle_range, config->rotation);

  lv_meter_indicator_t *outline = lv_meter_add_arc(meter, scale, OUTLINE_WIDTH, PALETTE_WHITE, OUTLINE_WIDTH);
  lv_meter_set_indicator_start_value(meter, outline, config->scale_min);
  lv_meter_set_indicator_end_value(meter, outline, config->scale_max);

  *config->indicator = lv_meter_add_needle_line(meter, scale, NEEDLE_WIDTH, NEEDLE_COLOR, NEEDLE_OFFSET);
}

void make_left_meter(void) {
  MeterConfig config = {&oil_meter, &oil_indic, BatteryData.min * BATTERY_SCALE, BatteryData.max * BATTERY_SCALE, 90, 135};
  make_meter(&config);
}

void make_right_meter(void) {
  MeterConfig config = {&water_meter, &water_indic, WaterData.max, WaterData.min, 90, 315};
  make_meter(&config);
}

void make_styles(void) {
  lv_style_init(&style_icon);
  lv_style_set_text_font(&style_icon, &font_awesome_icons_small);
  lv_style_set_text_color(&style_icon, PALETTE_GREY);
}

void make_inner_circle(void) {
  lv_obj_t *inner_circle = lv_obj_create(daily_scr);
  lv_obj_set_size(inner_circle, 312, 312);
  lv_obj_center(inner_circle);
  lv_obj_set_style_bg_color(inner_circle, PALETTE_BLACK, 0);
  lv_obj_set_style_bg_opa(inner_circle, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(inner_circle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(inner_circle, 0, 0);
}

void make_icons(void) {
  water_icon = lv_label_create(daily_scr);
  lv_label_set_text(water_icon, WATER_SYMBOL);
  lv_obj_add_style(water_icon, &style_icon, 0);
  lv_obj_align(water_icon, LV_ALIGN_CENTER, WaterData.horz_pos - 40, WaterData.vert_pos);

  battery_icon = lv_label_create(daily_scr);
  lv_label_set_text(battery_icon, BATTERY_SYMBOL);
  lv_obj_add_style(battery_icon, &style_icon, 0);
  lv_obj_align(battery_icon, LV_ALIGN_CENTER, BatteryData.horz_pos + 40, BatteryData.vert_pos);
}

void make_daily_screen(void) {
  daily_scr = startup_scr;
  lv_obj_set_style_bg_color(daily_scr, PALETTE_BLACK, 0);

  make_left_meter();
  make_right_meter();
  make_inner_circle();
  make_icons();
  create_green_masks();
}

void create_dual_gauge(void) {
  startup_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(startup_scr, PALETTE_BLACK, 0);
  lv_obj_set_style_bg_opa(startup_scr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(startup_scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(startup_scr, LV_SCROLLBAR_MODE_OFF);

  make_styles();
  daily_scr = startup_scr;
  make_daily_screen();
}

void update_water_temp(int new_water_temp) {
  // if (new_water_temp == -1 || new_water_temp == target_water_value) {
  //   return;
  // }

  target_water_value = new_water_temp;
  lv_obj_invalidate(water_meter);
}

void update_battery_voltage(float new_val) {
  // if (new_val == -1 || new_val == target_battery_value) {
  //   return;
  // } ///HERE!!!

  target_battery_value = new_val;
  lv_obj_invalidate(oil_meter);
}

void update_dual_gauge(float left_val, float right_val) {
  if (!ui_ready) {
    return;
  }

  update_battery_voltage(left_val);
  update_water_temp((int)right_val);
}

void battery_needle_tick(void) {
    if (!ui_ready) return;

    float force = (target_battery_value - current_battery_value) * 15.0f;
    battery_velocity *= 0.82f;
    battery_velocity += force * NEEDLE_DT;
    current_battery_value += battery_velocity;

    lv_meter_set_indicator_value(oil_meter, oil_indic,
        (int)(current_battery_value * BATTERY_SCALE));
}

void water_needle_tick(void) {
  if (!ui_ready) {
    return;
  }

  float force = (target_water_value - current_water_value) * 12.0f;
  water_velocity *= 0.85f;
  water_velocity += force * NEEDLE_DT;
  water_velocity = constrain(water_velocity, -50.0f, 50.0f);
  current_water_value += water_velocity;

  lv_meter_set_indicator_value(water_meter, water_indic, (int)current_water_value);
}

void updateTestMode(void) {
  if (millis() - lastTestUpdate < testUpdateInterval) {
    return;
  }

  lastTestUpdate = millis();

  if (voltageIncreasing) {
    testVoltage += voltageStep;
  } else {
    testVoltage -= voltageStep;
  }

  if (random(0, 20) == 0) {
    testVoltage += random(-10, 20) * 0.1f;
  }

  testVoltage = 13;

  if (testVoltage >= BatteryData.max) {
    testVoltage = BatteryData.max;
    voltageIncreasing = false;
  } else if (testVoltage <= BatteryData.min) {
    testVoltage = BatteryData.min;
    voltageIncreasing = true;
  }

  testVoltage = constrain(testVoltage, BatteryData.min, BatteryData.max);

  if (temperatureIncreasing) {
    testTemperature += temperatureStep;
  } else {
    testTemperature -= temperatureStep;
  }

  float tempJitter = random(-10, 10) * 0.1f;
  testTemperature += tempJitter;

  if (random(0, 25) == 0) {
    testTemperature += random(-10, 20) * 0.1f;
  }

  if (testTemperature >= WaterData.max) {
    testTemperature = WaterData.max;
    temperatureIncreasing = false;
  } else if (testTemperature <= WaterData.min) {
    testTemperature = WaterData.min;
    temperatureIncreasing = true;
  }

  testTemperature = constrain(testTemperature, WaterData.min, WaterData.max);
  update_dual_gauge(testVoltage, testTemperature);
}



void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void example_increase_lvgl_tick(void *arg) {
  (void)arg;
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void show_splash(void) {
  DEBUG_PRINTLN("create splash screen");

  splash_screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(splash_screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(splash_screen, LV_OPA_COVER, 0);

  DEBUG_PRINTLN("create image");
  splash_img = lv_img_create(splash_screen);

  DEBUG_PRINTLN("set src");
  lv_img_set_src(splash_img, &splash_0);

  DEBUG_PRINTLN("center");
  lv_obj_center(splash_img);
  lv_obj_set_pos(splash_img, 0, 0);

  DEBUG_PRINT("W=");
  DEBUG_PRINTLN(lv_obj_get_width(splash_img));
  DEBUG_PRINT("H=");
  DEBUG_PRINTLN(lv_obj_get_height(splash_img));

  DEBUG_PRINTLN("load screen");
  lv_scr_load(splash_screen);
  DEBUG_PRINTLN(lv_obj_get_child_cnt(splash_screen));
  DEBUG_PRINTLN(lv_img_get_zoom(splash_img));

  lv_obj_set_style_opa(splash_img, LV_OPA_TRANSP, 0);
  lv_obj_fade_in(splash_img, 300, 0);

  lv_timer_handler();
  lv_refr_now(NULL);
}

void create_green_masks(void) {
  mask_160 = lv_obj_create(daily_scr);
  lv_obj_set_size(mask_160, 16, 20);
  lv_obj_set_pos(mask_160, 116, 102);
  lv_obj_set_style_bg_color(mask_160, lv_color_make(0, 0, 0), 0);
  lv_obj_set_style_border_width(mask_160, 0, 0);
  lv_obj_set_style_radius(mask_160, 6, 0);
  lv_obj_clear_flag(mask_160, LV_OBJ_FLAG_SCROLLABLE);

  mask_100 = lv_obj_create(daily_scr);
  lv_obj_set_size(mask_100, 16, 20);
  lv_obj_set_pos(mask_100, 116, 344);
  lv_obj_set_style_bg_color(mask_100, lv_color_make(0, 0, 0), 0);
  lv_obj_set_style_border_width(mask_100, 0, 0);
  lv_obj_set_style_radius(mask_100, 6, 0);
  lv_obj_clear_flag(mask_100, LV_OBJ_FLAG_SCROLLABLE);
}

void handle_elm_init(void) {
  static unsigned long last_step_time = 0;

  if (elm_step >= INIT_COUNT) {
    boot_state = BOOT_READY;
    return;
  }

  if (millis() - last_step_time > 500) {
    sendELM(elmInitCmds[elm_step]);
    elm_step++;
    last_step_time = millis();
  }
}

void handle_obd_poll(void) {
  DEBUG_PRINTLN("handle_obd_poll");

  unsigned long now = millis();

  if (now - lastRequest > requestInterval && !obd_voltage_pending) {
    lastRequest = now;
    obd_voltage_due = now + OBD_REQUEST_GAP_MS;
    obd_voltage_pending = true;
    sendOBD("0105");
  }

  if (obd_voltage_pending && (long)(now - obd_voltage_due) >= 0) {
    sendOBD("0142");
    obd_voltage_pending = false;
  }

  while (Serial1.available()) {
    char c = Serial1.read();

    if (obdBufferPos < OBD_BUFFER_SIZE - 1) {
      obdBuffer[obdBufferPos++] = c;
    }

    if (c == '>') {
      obdBuffer[obdBufferPos] = '\0';
      parseOBDResponse(obdBuffer);
      update_dual_gauge(voltage, temperature);
      obdBufferPos = 0;
    }
  }

  if (obdBufferPos >= OBD_BUFFER_SIZE - 1) {
    obdBufferPos = 0;
  }
}

void setup() {
  current_battery_value = (float)BatteryData.min;
  current_water_value = (float)WaterData.min;
  target_water_value = WaterData.min;
  target_battery_value = BatteryData.min;

  Serial.begin(9600); // debug
  Serial1.begin(38400, SERIAL_8N1, 18, 17);

  randomSeed(analogRead(0));

  Wire.begin(IIC_SDA, IIC_SCL);
  gfx->begin();
  gfx->setBrightness(200);

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_WIDTH * 40);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  show_splash();

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  lv_indev_drv_register(&indev_drv);

  splash_start_time = millis();

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };

  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  DEBUG_PRINTLN("Setup complete...");
}

void switch_screen_cb(lv_timer_t *t) {
    lv_timer_del(t);
    
    // Nuke all animations
    lv_anim_del_all();
    
    // Delete splash
    if (splash_screen) {
        lv_obj_del_async(splash_screen);
        splash_screen = NULL;
        splash_img = NULL;
    }

    // Load gauge screen fully opaque, no tricks
    lv_obj_set_style_opa(startup_scr, LV_OPA_COVER, 0);
    lv_scr_load(startup_scr);

    ui_ready = true;
    boot_state = BOOT_DONE;
}

void transition_to_main_ui(void) {
    if (transition_started) return;
    transition_started = true;

    create_dual_gauge();

    lv_meter_set_indicator_value(oil_meter, oil_indic,
        (int)(current_battery_value * BATTERY_SCALE));
    lv_meter_set_indicator_value(water_meter, water_indic,
        (int)current_water_value);


    lv_obj_update_layout(startup_scr);

    // Just wait SPLASH_FADE_MS then hard switch — no fade animations at all
    lv_timer_create(switch_screen_cb, SPLASH_FADE_MS, NULL);
}

void loop() {
    lv_timer_handler();

    switch (boot_state) {
        case BOOT_SPLASH:
            boot_state = BOOT_INIT;  // always go to INIT, even in TEST_MODE
            break;

        case BOOT_INIT:
            #if TEST_MODE
                boot_state = BOOT_READY;  // skip ELM init in test mode
            #else
                handle_elm_init();
            #endif
            break;

        case BOOT_READY:
            if (millis() - splash_start_time > SPLASH_MIN_MS) {
                transition_to_main_ui();
                boot_state = BOOT_SPLASH;  // park here while timer runs
            }
            break;

        case BOOT_DONE:
            #if TEST_MODE
                updateTestMode();
            #else
                handle_obd_poll();
            #endif
            water_needle_tick();
            battery_needle_tick();
            break;
    }

    update_dual_gauge(voltage, temperature);
    delay(1);
}

void sendELM(const char *cmd) {
  DEBUG_PRINT(">> ");
  DEBUG_PRINTLN(cmd);
  Serial1.print(cmd);
  Serial1.print("\r");
}

void sendOBD(const char *pid) {
  DEBUG_PRINTLN(pid);

  Serial1.print(pid);
  Serial1.print("\r");
}

void parseOBDResponse(const char *resp) {
  char clean[OBD_BUFFER_SIZE];
  int clean_pos = 0;

  for (int i = 0; resp[i] != '\0' && clean_pos < OBD_BUFFER_SIZE - 1; i++) {
    char c = resp[i];

    if (c != ' ' && c != '\r' && c != '\n' && c != '>') {
      clean[clean_pos++] = c;
    }
  }

  clean[clean_pos] = '\0';

  char *coolant = strstr(clean, "4105");
  if (coolant && strlen(coolant) >= 6) {
    char hexVal[] = {coolant[4], coolant[5], '\0'};
    int A = strtol(hexVal, NULL, 16);
    temperature = A - 40;
  }

char *module_voltage = strstr(clean, "4142");
if (module_voltage && strlen(module_voltage) >= 8) {
    char hexVal1[] = {module_voltage[4], module_voltage[5], '\0'};
    char hexVal2[] = {module_voltage[6], module_voltage[7], '\0'};

    int A = strtol(hexVal1, NULL, 16);
    int B = strtol(hexVal2, NULL, 16);

    voltage = (256 * A + B) / 1000.0;

    DEBUG_PRINTLN(clean);      // 414224CC
    DEBUG_PRINTLN(voltage);    // 9.42
}
}
