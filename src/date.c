#include <pebble.h>
#include "date.h"
#include "weather.h"

static TextLayer *time_layer;
static TextLayer *date_layer;

void update_time();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

void date_startup(Window *window, GFont time_font, GFont date_font) {
  GColor color = GColorWhite;
  
  time_layer = text_layer_create(GRect(0, 25, 144, 50));
  if (time_layer != NULL) {
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_color(time_layer, color);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    text_layer_set_font(time_layer, time_font);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
  }

  date_layer = text_layer_create(GRect(0, 65, 144, 20));
  if (date_layer != NULL) {
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_color(date_layer, color);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_font(date_layer, date_font);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
  }

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  update_time();
}

void date_teardown() {
  tick_timer_service_unsubscribe();
  
  if (date_layer != NULL) {
    text_layer_destroy(date_layer);
    date_layer = NULL;
  }
  if (time_layer != NULL) {
    text_layer_destroy(time_layer);
    time_layer = NULL;
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  if(tick_time->tm_sec == 0) {
    weather_on_minute(tick_time);
  }

  // Create a long-lived buffer
  static char time_buffer[] = "00:00:00";

  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tick_time);
  } else {
    // Use 12 hour format
    strftime(time_buffer, sizeof(time_buffer), "%I:%M:%S", tick_time);
  }
  if (time_layer != NULL) {
    text_layer_set_text(time_layer, time_buffer);
  }

  static char date_buffer[] = "00-00-0000";
  strftime(date_buffer, sizeof(date_buffer), "%m-%d-%Y", tick_time);
  if (date_layer != NULL) {
    text_layer_set_text(date_layer, date_buffer);
  }
}