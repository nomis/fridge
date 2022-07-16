/*
 * fridge - Fridge Controller
 * Copyright 2019,2022  Simon Arlott
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

#include "fridge/app.h"

#include <Arduino.h>

#include <memory>
#include <vector>

#include <uuid/common.h>
#include <uuid/console.h>
#include <uuid/log.h>
#include <uuid/syslog.h>
#include <uuid/telnet.h>

#include "app/config.h"
#include "app/console.h"
#include "app/network.h"
#include "fridge/sensors.h"

static const char __pstr__enabled[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "enabled";
static const char __pstr__disabled[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "disabled";

namespace fridge {

void App::start() {
	pinMode(BUZZER_PIN, OUTPUT);
	digitalWrite(BUZZER_PIN, HIGH);

	pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, LOW);

	app::App::start();

	relay(false);

	sensors_.start(SENSOR_PIN);

	buzzer(false);
}

void App::loop() {
	app::App::loop();

	sensors_.loop();
}

void App::relay(bool value) {
	logger_.debug(F("Relay %S"), value ? __pstr__enabled : __pstr__disabled);
	digitalWrite(RELAY_PIN, value ? HIGH : LOW);
}

void App::buzzer(bool value) {
	logger_.debug(F("Buzzer %S"), value ? __pstr__enabled : __pstr__disabled);
	digitalWrite(BUZZER_PIN, value ? HIGH : LOW);
}

const std::vector<fridge::Sensors::Device> App::sensor_devices() {
	return sensors_.devices();
}

} // namespace fridge
