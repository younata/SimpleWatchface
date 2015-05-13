#pragma once

#include <pebble.h>

void weather_startup(Window *window, GFont weather_font, GFont status_font);
void weather_teardown();

void weather_on_minute(struct tm *now);