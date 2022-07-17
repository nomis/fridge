/*
 * fridge - Fridge Controller
 * Copyright 2022  Simon Arlott
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

#include "fridge/door.h"

#include <Arduino.h>

#include <uuid/log.h>

static const char __pstr__logger_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "door";

using uuid::log::Level;

namespace fridge {

uuid::log::Logger Door::logger_{FPSTR(__pstr__logger_name), uuid::log::Facility::DAEMON};

void Door::start(int pin) {
	pin_ = pin;
	pinMode(pin_, INPUT_PULLUP);
}

void Door::loop() {
	State state = digitalRead(pin_) == LOW ? State::OPEN : State::CLOSED;

	if (state == stable_state_) {
		new_state_ = State::UNKNOWN;
	} else if (state == new_state_) {
		if (millis() - last_activity_ >= DEBOUNCE_INTERVAL_MS) {
			stable_state_ = state;

			if (stable_state_ == State::OPEN) {
				logger_.notice(F("Door open"));
			} else {
				logger_.notice(F("Door closed"));
			}
		}
	} else {
		new_state_ = state;
		last_activity_ = millis();
	}
}

} // namespace fridge
