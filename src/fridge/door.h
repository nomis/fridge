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

#pragma once

#include <Arduino.h>

#include <uuid/log.h>

namespace fridge {

class Door {
public:
	Door() = default;
	~Door() = default;

	void start(int pin);
	void loop();

private:
	enum class State {
		UNKNOWN,
		OPEN,
		CLOSED,
	};

	static constexpr unsigned long DEBOUNCE_INTERVAL_MS = 50;

	static uuid::log::Logger logger_;

	int pin_;
	unsigned long last_activity_ = millis();
	State stable_state_ = State::UNKNOWN;
	State new_state_ = State::UNKNOWN;
};

} // namespace fridge
