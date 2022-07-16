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

#include <string>
#include <vector>

#include <uuid/log.h>
#include <OneWire.h>

namespace fridge {

class Sensors {
public:
	class Device {
	public:
		Device(const uint8_t addr[]);
		~Device() = default;

		uint64_t id() const;
		std::string to_string() const;

		float temperature_c_ = NAN;

	private:
		const uint64_t id_;
	};

	Sensors() = default;
	~Sensors() = default;

	void start(int pin);
	void loop();

	const std::vector<Device> devices() const;

private:
	enum class State {
		IDLE,
		READING,
		SCANNING
	};

	static constexpr size_t ADDR_LEN = 8;

	static constexpr size_t SCRATCHPAD_LEN = 9;
	static constexpr size_t SCRATCHPAD_TEMP_MSB = 1;
	static constexpr size_t SCRATCHPAD_TEMP_LSB = 0;
	static constexpr size_t SCRATCHPAD_CONFIG = 4;

	static constexpr uint8_t TYPE_DS18B20 = 0x28;

	static constexpr unsigned long READ_INTERVAL_MS = 1000;
	static constexpr unsigned long READ_TIMEOUT_MS = 2000;
	static constexpr unsigned long SCAN_TIMEOUT_MS = 30000;

	static constexpr uint8_t CMD_CONVERT_TEMP = 0x44;
	static constexpr uint8_t CMD_READ_SCRATCHPAD = 0xBE;

	static uuid::log::Logger logger_;

	bool temperature_convert_complete();
	float get_temperature_c(const uint8_t addr[]);

	OneWire bus_;
	unsigned long last_activity_ = millis();
	State state_ = State::IDLE;
	std::vector<Device> found_;
	std::vector<Device> devices_;
};

} // namespace fridge
