#include <pebble.h>
#include "weather.h"
  
enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1,
};

enum {
  KEY_STORE_WEATHER = 0,
  KEY_STORE_WEATHER_LAST_CHECK = 1,
};
typedef struct Weather_Object {
  uint32_t temperature;
  char conditions[32];
} Weather_Object;

static TextLayer *weather_layer;
static TextLayer *status_layer;

static time_t last_check = 0;

void weather_askForUpdate();

static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
static void update_weather_layer_text(int temperature, char *conditions);
static void update_status_layer_text();

void weather_startup(Window *window, GFont weather_font, GFont status_font) {
  weather_layer = text_layer_create(GRect(0, 85, 144, 20));
  if (weather_layer != NULL) {
    text_layer_set_background_color(weather_layer, GColorClear);
    text_layer_set_text_color(weather_layer, GColorWhite);
    text_layer_set_text_alignment(weather_layer, GTextAlignmentCenter);
    text_layer_set_font(weather_layer, weather_font);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_layer));
  } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to create weather_layer");
  }

  status_layer = text_layer_create(GRect(0, 105, 144, 20));
  if (status_layer != NULL) {
    text_layer_set_background_color(status_layer, GColorClear);
    text_layer_set_text_color(status_layer, GColorWhite);
    text_layer_set_text_alignment(status_layer, GTextAlignmentCenter);
    text_layer_set_font(status_layer, status_font);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(status_layer));
  } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to create status_layer");
  }

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
  if (persist_exists(KEY_STORE_WEATHER_LAST_CHECK)) {
    last_check = persist_read_int(KEY_STORE_WEATHER_LAST_CHECK);
  }
}

void weather_teardown() {
  if (weather_layer != NULL) {
    text_layer_destroy(weather_layer);
    weather_layer = NULL;
  }
  if (status_layer != NULL) {
    text_layer_destroy(status_layer);
    status_layer = NULL;
  }
  app_message_deregister_callbacks();
}

void weather_update(struct tm *now) {
  if(now->tm_min % 30 == 0 && now->tm_sec) {
    weather_askForUpdate();
  }
  update_status_layer_text();
}

void weather_askForUpdate() {
  if (weather_layer == NULL) { return; }
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);

  // Send the message!
  app_message_outbox_send();
}

static void update_weather_layer_text(int temperature, char *conditions) {
  static char weather_layer_buffer[40];

  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%dF, %s", temperature, conditions);
  if (weather_layer != NULL) {
    text_layer_set_text(weather_layer, weather_layer_buffer);
  }
}

static void update_status_layer_text() {
  if (last_check == 0 || status_layer == NULL) { return; }
  time_t time_difference = abs(time(NULL) - last_check);
  int seconds = time_difference % 60;
  const char *text = text_layer_get_text(status_layer);
  if (seconds != 0 && (text == NULL || strlen(text_layer_get_text(status_layer)) != 0)) {
    return;
  }
  time_t time_difference_in_minutes = time_difference / 60;
  int minutes = time_difference_in_minutes % 60;
  int hours = time_difference_in_minutes / 60;

  static char status_layer_buffer[40];
  char hours_buffer[24] = "";
  if (hours > 0) {
    snprintf(hours_buffer, sizeof(hours_buffer), "%d hours, ", hours);
  }
  char minutes_buffer[16] = "";
  if (minutes == 1) {
    snprintf(minutes_buffer, sizeof(minutes_buffer), "1 minute");
  } else {
    snprintf(minutes_buffer, sizeof(minutes_buffer), "%d minutes", minutes);
  }
  snprintf(status_layer_buffer, sizeof(status_layer_buffer), "%s%s", hours_buffer, minutes_buffer);
  text_layer_set_text(status_layer, status_layer_buffer);

  #ifdef PBL_COLOR
  GColor color = GColorWhite;
  if (hours == 0 && minutes > 35) {
    color = GColorYellow;
  } else if (hours >= 1) {
    color = GColorRed;
  } else {
    color = GColorGreen;
  }
  text_layer_set_text_color(status_layer, color);
  #endif
}

static void set_status_error(char *message) {
  persist_write_int(KEY_STORE_WEATHER_LAST_CHECK, 0);
  last_check = 0;

  if (status_layer == NULL) { return; }

  if (message != NULL) {
    text_layer_set_text(status_layer, message);
  }

  #ifdef PBL_COLOR
  text_layer_set_text_color(status_layer, GColorDarkCandyAppleRed);
  #endif
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
  persist_write_int(KEY_STORE_WEATHER_LAST_CHECK, time(NULL));
  update_weather_layer_text(weather_object.temperature, weather_object.conditions);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  set_status_error("Error receiving update");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  set_status_error("Error requesting update");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
}