#pragma once

#include <pebble.h>

void weather_startup(Window *window, GFont weather_font, GFont status_font);
void weather_teardown();

void weather_update(struct tm *now);