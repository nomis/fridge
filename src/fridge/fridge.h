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

#ifndef FRIDGE_FRIDGE_H_
#define FRIDGE_FRIDGE_H_

#include <Arduino.h>

#include <memory>

#include <uuid/syslog.h>

#include "fridge/console.h"
#include "fridge/network.h"

namespace fridge {

class Fridge {
private:
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) || defined(ESP8266_WEMOS_D1MINI)
	static constexpr unsigned long SERIAL_CONSOLE_BAUD_RATE = 115200;
	static constexpr auto& serial_console_ = Serial;

	static constexpr int RELAY_PIN = 13; /* D7 */
	static constexpr int SENSOR_PIN = 12; /* D6 */
	static constexpr int BUZZER_PIN = 14; /* D5 */
#else
# error "Unknown board"
#endif

public:
	static void start();
	static void loop();

	static void relay(bool value);
	static void buzzer(bool value);

	static void config_syslog();

private:
	Fridge() = delete;

	static void shell_prompt();

	static uuid::log::Logger logger_;
	static fridge::Network network_;
	static uuid::syslog::SyslogService syslog_;
	static std::shared_ptr<fridge::FridgeShell> shell_;
};

} // namespace fridge

#endif
