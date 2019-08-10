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

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ESP8266_WEMOS_D1MINI)
static constexpr auto& serial_console = Serial;
static constexpr unsigned long SERIAL_CONSOLE_BAUD_RATE = 115200;
static constexpr int RELAY_PIN = 13; /* D7 */
static constexpr int SENSOR_PIN = 12; /* D6 */
#endif

#endif
