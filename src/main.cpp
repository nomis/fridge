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

#include "main.h"

#include <Arduino.h>

#include <memory>

#include <uuid/common.h>
#include <uuid/console.h>
#include <uuid/log.h>

#include "console.h"
#include "config.h"

static const char __pstr__logger_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "main";
static uuid::log::Logger logger_{FPSTR(__pstr__logger_name), uuid::log::Facility::KERN};

static void shell_prompt() {
	serial_console.println();
	serial_console.println(F("Press ^C to activate this console"));
}

void setup() {
	logger_.info(F("System startup (fridge " FRIDGE_REVISION ")"));
	logger_.info(F("Reset: %s"), ESP.getResetInfo().c_str());

	pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, LOW);

	serial_console.begin(SERIAL_CONSOLE_BAUD_RATE);
	serial_console.println();
	serial_console.println(F("fridge " FRIDGE_REVISION));

	shell_prompt();
}

void loop() {
	static std::shared_ptr<fridge::FridgeShell> shell;

	uuid::loop();
	uuid::console::Shell::loop_all();

	if (shell) {
		if (!shell->running()) {
			shell.reset();
			shell_prompt();
		}
	} else {
		int c = serial_console.read();
		if (c == '\x03' || c == '\x0C') {
			shell = std::make_shared<fridge::FridgeStreamConsole>(serial_console, c == '\x0C');
			shell->start();
		}
	}

	::yield();
}
