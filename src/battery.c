#include <pebble.h>
#include "battery.h"

static TextLayer *battery_layer;

static void battery_handler(BatteryChargeState charge);

void battery_startup(Window *window, GFont font) {
  battery_layer = text_layer_create(GRect(0, 105, 120, 24));
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);

  text_layer_set_font(battery_layer, font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(battery_layer));
  
  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());

}

void battery_teardown() {
  text_layer_destroy(battery_layer);

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
      #ifdef PBL_COLOR
        GColor color = charge.charge_percent == 100 ? GColorGreen : GColorCyan;
        text_layer_set_text_color(battery_layer, color);
      #endif
    } else {
      strncpy(battery_buffer, "Not Charging", sizeof(battery_buffer));
    }
  } else {
    snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", charge.charge_percent);
    #ifdef PBL_COLOR
      GColor color = charge.charge_percent <= 20 ? GColorRed : GColorGreen;
      text_layer_set_text_color(battery_layer, color);
    #endif
  }
  text_layer_set_text(battery_layer, battery_buffer);
}