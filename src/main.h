/*
 * fridge - Fridge Controller
 * Copyright 2019  Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FRIDGE_SRC_MAIN_H_
#define FRIDGE_SRC_MAIN_H_

#include <Arduino.h>

#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
constexpr auto *output = &Serial;
constexpr unsigned long OUTPUT_BAUD_RATE = 115200;
#endif

#endif
