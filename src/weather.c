#include <pebble.h>
#include "weather.h"
  
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
};

enum {
  KEY_STORE_WEATHER = 0,
};
typedef struct Weather_Object {
  uint32_t temperature;
  char conditions[32];
} Weather_Object;

static TextLayer *weather_layer;
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
static void update_weather_layer_text(int temperature, char *conditions);

void weather_startup(Window *window, GFont font) {
  weather_layer = text_layer_create(GRect(0, 85, 144, 20));
  text_layer_set_background_color(weather_layer, GColorClear);
  text_layer_set_text_color(weather_layer, GColorWhite);
  text_layer_set_text_alignment(weather_layer, GTextAlignmentCenter);
  
  text_layer_set_font(weather_layer, font);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_layer));

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  if (persist_exists(KEY_STORE_WEATHER)) {
    Weather_Object weather_object;
    persist_read_data(KEY_STORE_WEATHER, &weather_object, sizeof(weather_object));
    update_weather_layer_text(weather_object.temperature, weather_object.conditions);
  }
}

void weather_teardown() {
  text_layer_destroy(weather_layer);
  app_message_deregister_callbacks();
}

void weather_askForUpdate() {
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  APP_LOG(APP_LOG_LEVEL_INFO, "Asking for weather update");

  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);

  // Send the message!
  app_message_outbox_send();
}

static void update_weather_layer_text(int temperature, char *conditions) {
  static char weather_layer_buffer[40];

  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%dF, %s", temperature, conditions);
  text_layer_set_text(weather_layer, weather_layer_buffer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  Weather_Object weather_object;

  // For all items
  while(t != NULL) {
    switch(t->key) {
    case KEY_TEMPERATURE:
      weather_object.temperature = t->value->int32;
      break;
    case KEY_CONDITIONS:
      snprintf(weather_object.conditions, sizeof(weather_object.conditions), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  persist_write_data(KEY_STORE_WEATHER, &weather_object, sizeof(weather_object));
  update_weather_layer_text(weather_object.temperature, weather_object.conditions);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}