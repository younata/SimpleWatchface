#include <pebble.h>

#include "date.h"
#include "weather.h"
#include "battery.h"
  
static Window *main_window;

static GFont header_font;
static GFont subheader_font;
static GFont small_font;

static void main_window_load(Window *window) {
  // Create fonts
  header_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_NEUE_LIGHT_36));
  subheader_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_NEUE_LIGHT_18));
  small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_NEUE_LIGHT_12));
  
  if (header_font != NULL && subheader_font != NULL) {
    date_startup(window, header_font, subheader_font);
  }
  if (subheader_font != NULL && small_font != NULL) {
    weather_startup(window, subheader_font, small_font);
  }
  if (small_font != NULL) {
    battery_startup(window, small_font);
  }
}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(header_font);
  fonts_unload_custom_font(subheader_font);
  fonts_unload_custom_font(small_font);

  date_teardown();
  weather_teardown();
  battery_teardown();
}

static void init() {
  main_window = window_create();
  if (main_window != NULL) {
  
    window_set_background_color(main_window, GColorBlack);

    window_set_window_handlers(main_window, (WindowHandlers) {
      .load = main_window_load,
      .unload = main_window_unload
    });

    window_stack_push(main_window, true);
  }
}

static void deinit() {
  window_destroy(main_window);
}

int main(int argc, char *argv[]) {
  init();
  if (main_window != NULL) {
    app_event_loop();
    deinit();
  } else {
    return 1;
  }
}