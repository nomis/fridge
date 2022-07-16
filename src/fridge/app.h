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

#pragma once

#include <Arduino.h>

#include <initializer_list>
#include <memory>
#include <vector>

#include <uuid/syslog.h>
#include <uuid/telnet.h>

#include "../app/app.h"
#include "../app/console.h"
#include "../app/network.h"
#include "sensors.h"

namespace fridge {

class App: public app::App {
private:
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ESP8266_WEMOS_D1MINI)
	static constexpr int RELAY_PIN = 13; /* D7 */
	static constexpr int SENSOR_PIN = 12; /* D6 */
	static constexpr int BUZZER_PIN = 14; /* D5 */
#elif defined(ARDUINO_LOLIN_S2_MINI)
	static constexpr int RELAY_PIN = 13;
	static constexpr int SENSOR_PIN = 12;
	static constexpr int BUZZER_PIN = 14;
#else
# error "Unknown board"
#endif

public:
	void start() override;
	void loop() override;

	void relay(bool value);
	void buzzer(bool value);

	const std::vector<Sensors::Device> sensor_devices();

private:
	Sensors sensors_;
};

} // namespace fridge
