#include <pebble.h>
#include "battery.h"

static TextLayer *battery_layer;
static Layer *charge_layer;

static void battery_handler(BatteryChargeState charge);
GColor color_from_battery_state(BatteryChargeState state);
static void battery_layer_draw(Layer *layer, GContext *ctx);

void battery_startup(Window *window, GFont font) {
  battery_layer = text_layer_create(GRect(0, 5, 115, 15));
  if (battery_layer != NULL) {
    text_layer_set_background_color(battery_layer, GColorClear);
    text_layer_set_text_color(battery_layer, GColorWhite);
    text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);
    text_layer_set_font(battery_layer, font);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(battery_layer));
  } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to create battery_layer");
  }

  charge_layer = layer_create(GRect(119, 8, 20, 10));
  if (charge_layer != NULL) {
    layer_set_update_proc(charge_layer, battery_layer_draw);
    layer_add_child(window_get_root_layer(window), charge_layer);
  } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to create charge_layer");
  }
  
  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());
}

void battery_teardown() {
  if (battery_layer != NULL) {
    text_layer_destroy(battery_layer);
    battery_layer = NULL;
  }

  if (charge_layer != NULL) {
    layer_destroy(charge_layer);
    charge_layer = NULL;
  }

  battery_state_service_unsubscribe();
}

static void battery_handler(BatteryChargeState charge) {
  static char battery_buffer[15];
  
  if (charge.is_plugged) {
    if (charge.is_charging) {
      if (charge.charge_percent == 100) {
        strncpy(battery_buffer, "Charged", sizeof(battery_buffer));
      } else {
        strncpy(battery_buffer, "Charging", sizeof(battery_buffer));
      }
    } else {
      strncpy(battery_buffer, "Not Charging", sizeof(battery_buffer));
    }
  } else {
    snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", charge.charge_percent);
  }
  if (battery_layer != NULL) {
    text_layer_set_text_color(battery_layer, color_from_battery_state(charge));
    text_layer_set_text(battery_layer, battery_buffer);
  }
  if (charge_layer != NULL) {
    layer_mark_dirty(charge_layer);
  }
}

GColor color_from_battery_state(BatteryChargeState state) {
  #ifdef PBL_COLOR
    GColor color = GColorGreen;
    if (state.charge_percent <= 20) {
      color = GColorRed;
    } else if (state.is_plugged && !state.is_charging) {
      color = GColorCyan;
    }
    return color;
  #else
    return GColorWhite;
  #endif
}

static void battery_layer_draw(Layer *layer, GContext *ctx) {
  if (layer == NULL) { return; }
  GRect bounds = layer_get_bounds(layer);

  BatteryChargeState battery_state = battery_state_service_peek();

  uint8_t stroke_width = 1;
  uint8_t stroke_width2 = stroke_width + 1;

  GColor color = color_from_battery_state(battery_state);
  
  graphics_context_set_stroke_color(ctx, color);
  #ifdef PBL_COLOR
    graphics_context_set_stroke_width(ctx, stroke_width);
  #else
    GRect inner = bounds;
    for (int i = 0; i < stroke_width - 1; i++) {
      inner = grect_crop(inner, 1);
      graphics_draw_rect(ctx, inner);
    }
  #endif
  graphics_draw_rect(ctx, bounds);

  GRect battery = GRect(stroke_width2,
                        stroke_width2,
                        ((bounds.size.w - (2 * stroke_width2)) * ((float)battery_state.charge_percent / 100.0)),
                        bounds.size.h - (2 * stroke_width2));
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, battery, 0, GCornerNone);
}