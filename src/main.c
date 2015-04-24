#include <pebble.h>
  
static Window *main_window;
static TextLayer *time_layer;
static TextLayer *date_layer;
static TextLayer *weather_layer;
static TextLayer *battery_layer;

static GFont header_font;
static GFont subheader_font;

enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1
};

static void update_time();
static void battery_handler(BatteryChargeState charge);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);

static void main_window_load(Window *window) {
  // Create time TextLayer
  time_layer = text_layer_create(GRect(0, 25, 144, 50));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  
  // Create header_font
  header_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_NEUE_LIGHT_36));
  text_layer_set_font(time_layer, header_font);
  
  date_layer = text_layer_create(GRect(0, 65, 144, 20));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  
  subheader_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_HELVETICA_NEUE_LIGHT_18));
  text_layer_set_font(date_layer, subheader_font);
  
  // Set up the weatherlayer
  weather_layer = text_layer_create(GRect(0, 85, 144, 20));
  text_layer_set_background_color(weather_layer, GColorClear);
  text_layer_set_text_color(weather_layer, GColorWhite);
  text_layer_set_text_alignment(weather_layer, GTextAlignmentCenter);
  
  text_layer_set_font(weather_layer, subheader_font);

  battery_layer = text_layer_create(GRect(0, 105, 120, 20));
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);

  text_layer_set_font(battery_layer, subheader_font);
  battery_handler(battery_state_service_peek());

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(battery_layer));

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void main_window_unload(Window *window) {
  text_layer_destroy(time_layer);
  fonts_unload_custom_font(header_font);

  text_layer_destroy(date_layer);

  text_layer_destroy(weather_layer);
  fonts_unload_custom_font(subheader_font);

  text_layer_destroy(battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
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

  text_layer_set_text(battery_layer, battery_buffer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);
  
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[40];

  // For all items
  while(t != NULL) {
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dF", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(weather_layer, weather_layer_buffer);
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

static void init() {
  // Create main Window element and assign to pointer
  main_window = window_create();
  
  window_set_background_color(main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(main_window, true);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  update_time();
}

static void deinit() {
  window_destroy(main_window);

  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(int argc, char *argv[]) {
  init();
  app_event_loop();
  deinit();
}