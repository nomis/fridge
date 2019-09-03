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

#include "fridge/fridge.h"

#include <Arduino.h>

#include <memory>

#include <uuid/common.h>
#include <uuid/console.h>
#include <uuid/log.h>
#include <uuid/syslog.h>

#include "fridge/config.h"
#include "fridge/console.h"
#include "fridge/network.h"

static const char __pstr__logger_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "fridge";
static const char __pstr__enabled[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "enabled";
static const char __pstr__disabled[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "disabled";

namespace fridge {

uuid::log::Logger Fridge::logger_{FPSTR(__pstr__logger_name), uuid::log::Facility::KERN};
fridge::Network Fridge::network_;
uuid::syslog::SyslogService Fridge::syslog_;
std::shared_ptr<FridgeShell> Fridge::shell_;

void Fridge::start() {
	pinMode(BUZZER_PIN, OUTPUT);
	digitalWrite(BUZZER_PIN, HIGH);

	pinMode(RELAY_PIN, OUTPUT);
	digitalWrite(RELAY_PIN, LOW);

	syslog_.start();

	logger_.info(F("System startup (fridge " FRIDGE_REVISION ")"));
	logger_.info(F("Reset: %s"), ESP.getResetInfo().c_str());

	relay(false);

	serial_console_.begin(SERIAL_CONSOLE_BAUD_RATE);
	serial_console_.println();
	serial_console_.println(F("fridge " FRIDGE_REVISION));

	network_.start();
	config_syslog();
	shell_prompt();

	buzzer(false);
}

void Fridge::loop() {
	uuid::loop();
	syslog_.loop();
	uuid::console::Shell::loop_all();

	if (shell_) {
		if (!shell_->running()) {
			shell_.reset();
			shell_prompt();
		}
	} else {
		int c = serial_console_.read();
		if (c == '\x03' || c == '\x0C') {
			shell_ = std::make_shared<FridgeStreamConsole>(serial_console_, c == '\x0C');
			shell_->start();
		}
	}
}

void Fridge::shell_prompt() {
	serial_console_.println();
	serial_console_.println(F("Press ^C to activate this console"));
}

void Fridge::relay(bool value) {
	logger_.debug(F("Relay %S"), value ? __pstr__enabled : __pstr__disabled);
	digitalWrite(RELAY_PIN, value ? HIGH : LOW);
}

void Fridge::buzzer(bool value) {
	logger_.debug(F("Buzzer %S"), value ? __pstr__enabled : __pstr__disabled);
	digitalWrite(BUZZER_PIN, value ? HIGH : LOW);
}

void Fridge::config_syslog() {
	Config config;
	IPAddress addr;

	if (!addr.fromString(config.get_syslog_host().c_str())) {
		addr = (uint32_t)0;
	}

	syslog_.hostname(config.get_hostname());
	syslog_.log_level(config.get_syslog_level());
	syslog_.mark_interval(config.get_syslog_mark_interval());
	syslog_.destination(addr);
}

} // namespace fridge
