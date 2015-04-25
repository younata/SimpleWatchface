#pragma once

#include <pebble.h>

void weather_startup(Window *window, GFont font);
void weather_teardown();

void weather_askForUpdate();