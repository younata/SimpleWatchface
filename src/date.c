#include <pebble.h>
#include "date.h"

static TextLayer *time_layer;
static TextLayer *date_layer;

static void update_time();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

void date_startup(Window *window, GFont time_font, GFont date_font) {
  time_layer = text_layer_create(GRect(0, 25, 144, 50));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_font(time_layer, time_font);

  date_layer = text_layer_create(GRect(0, 65, 144, 20));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_font(date_layer, date_font);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  update_time();
}

void date_teardown() {
  tick_timer_service_unsubscribe();
  text_layer_destroy(date_layer);
  text_layer_destroy(time_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
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
  text_layer_set_text(time_layer, time_buffer);
  
  static char date_buffer[] = "00-00-0000";
  strftime(date_buffer, sizeof(date_buffer), "%m-%d-%Y", tick_time);
  text_layer_set_text(date_layer, date_buffer);
}