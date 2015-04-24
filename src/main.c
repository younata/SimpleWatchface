#include <pebble.h>

#include "date.h"
#include "weather.h"
#include "battery.h"
  
static Window *main_window;

static GFont header_font;
static GFont subheader_font;

static void main_window_load(Window *window) {
  // Create fonts
  header_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_NEUE_LIGHT_36));
  subheader_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_NEUE_LIGHT_18));
  
  date_startup(window, header_font, subheader_font);
  weather_startup(window, subheader_font);
  battery_startup(window, subheader_font);
}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(header_font);
  fonts_unload_custom_font(subheader_font);

  date_teardown();
  weather_teardown();
  battery_teardown();
}

static void init() {
  main_window = window_create();
  
  window_set_background_color(main_window, GColorBlack);

  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(main_window, true);
}

static void deinit() {
  window_destroy(main_window);
}

int main(int argc, char *argv[]) {
  init();
  app_event_loop();
  deinit();
}