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

#include "fridge/sensors.h"

#include <Arduino.h>

#include <string>
#include <vector>

#include <uuid/log.h>

static const char __pstr__logger_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "sensors";

using uuid::log::Level;

namespace fridge {

uuid::log::Logger Sensors::logger_{FPSTR(__pstr__logger_name), uuid::log::Facility::DAEMON};

void Sensors::start(int pin) {
	bus_.begin(pin);
}

void Sensors::loop() {
	if (state_ == State::IDLE) {
		if (millis() - last_activity_ >= READ_INTERVAL_MS) {
			logger_.trace(F("Read temperature"));
			if (bus_.reset()) {
				bus_.skip();
				bus_.write(CMD_CONVERT_TEMP);

				state_ = State::READING;
			} else {
				logger_.err(F("Bus reset failed"));
			}
			last_activity_ = millis();
		}
	} else if (state_ == State::READING) {
		if (temperature_convert_complete()) {
			logger_.trace(F("Scan bus for devices"));
			bus_.reset_search();
			found_.clear();

			state_ = State::SCANNING;
			last_activity_ = millis();
		} else if (millis() - last_activity_ > READ_TIMEOUT_MS) {
			logger_.err(F("Temperature read timeout"));

			state_ = State::IDLE;
			last_activity_ = millis();
		}
	} else if (state_ == State::SCANNING) {
		if (millis() - last_activity_ > SCAN_TIMEOUT_MS) {
			logger_.err(F("Device scan timeout"));
			state_ = State::IDLE;
			last_activity_ = millis();
		} else {
			uint8_t addr[ADDR_LEN] = { 0 };

			if (bus_.search(addr)) {
				bus_.depower();

				if (bus_.crc8(addr, ADDR_LEN - 1) == addr[ADDR_LEN - 1]) {
					switch (addr[0]) {
					case TYPE_DS18B20:
						found_.emplace_back(addr);

						if (logger_.enabled(Level::TRACE)) {
							logger_.trace(F("Found device %s"), found_.back().to_string().c_str());
						}
						found_.back().temperature_c_ = get_temperature_c(addr);
						logger_.debug(F("Temperature of %s = %.2fC"), found_.back().to_string().c_str(), found_.back().temperature_c_);
						break;

					default:
						if (logger_.enabled(Level::TRACE)) {
							logger_.trace(F("Unknown device %s"), Device(addr).to_string().c_str());
						}
						break;
					}
				} else {
					if (logger_.enabled(Level::TRACE)) {
						logger_.trace(F("Invalid device %s"), Device(addr).to_string().c_str());
					}
				}
			} else {
				bus_.depower();
				devices_ = std::move(found_);
				found_.clear();

				if (logger_.enabled(Level::TRACE)) {
					if (devices_.size() == 1) {
						logger_.trace(F("Found 1 device"));
					} else {
						logger_.trace(F("Found %zu devices"), devices_.size());
					}
				}

				state_ = State::IDLE;
				last_activity_ = millis();
			}
		}
	}
}

bool Sensors::temperature_convert_complete() {
	return bus_.read_bit() == 1;
}

float Sensors::get_temperature_c(const uint8_t addr[]) {
	if (!bus_.reset()) {
		logger_.err(F("Bus reset failed before reading scratchpad from %s"),
				Device(addr).to_string().c_str());
		return NAN;
	}

	uint8_t scratchpad[SCRATCHPAD_LEN] = { 0 };

	bus_.select(addr);
	bus_.write(CMD_READ_SCRATCHPAD);
	bus_.read_bytes(scratchpad, SCRATCHPAD_LEN);

	if (!bus_.reset()) {
		logger_.err(F("Bus reset failed after reading scratchpad from %s"),
				Device(addr).to_string().c_str());
		return NAN;
	}

	if (bus_.crc8(scratchpad, SCRATCHPAD_LEN - 1) != scratchpad[SCRATCHPAD_LEN - 1]) {
		logger_.warning(F("Invalid scratchpad CRC: %02X%02X%02X%02X%02X%02X%02X%02X%02X from device %s"),
				scratchpad[0], scratchpad[1], scratchpad[2], scratchpad[3],
				scratchpad[4], scratchpad[5], scratchpad[6], scratchpad[7],
				scratchpad[8], Device(addr).to_string().c_str());
		return NAN;
	}

	int16_t raw_value = ((int16_t)scratchpad[SCRATCHPAD_TEMP_MSB] << 8) | scratchpad[SCRATCHPAD_TEMP_LSB];

	// Adjust based on device resolution
	int resolution = 9 + ((scratchpad[SCRATCHPAD_CONFIG] >> 5) & 0x3);
	switch (resolution) {
	case 9:
		raw_value &= ~0x1;
		break;

	case 10:
		raw_value &= ~0x3;
		break;

	case 11:
		raw_value &= ~0x7;
		break;

	case 12:
		break;
	}

	return (float)raw_value / 16;
}

const std::vector<Sensors::Device> Sensors::devices() const {
	return devices_;
}

Sensors::Device::Device(const uint8_t addr[])
		: id_(((uint64_t)addr[0] << 56)
				| ((uint64_t)addr[1] << 48)
				| ((uint64_t)addr[2] << 40)
				| ((uint64_t)addr[3] << 32)
				| ((uint64_t)addr[4] << 24)
				| ((uint64_t)addr[5] << 16)
				| ((uint64_t)addr[6] << 8)
				| (uint64_t)addr[7]) {

}

uint64_t Sensors::Device::id() const {
	return id_;
}

std::string Sensors::Device::to_string() const {
	std::string str(20, '\0');

	::snprintf_P(&str[0], str.capacity() + 1,
			PSTR("%02X-%04X-%04X-%04X-%02X"),
			(unsigned int)(id_ >> 56) & 0xFF,
			(unsigned int)(id_ >> 40) & 0xFFFF,
			(unsigned int)(id_ >> 24) & 0xFFFF,
			(unsigned int)(id_ >> 8) & 0xFFFF,
			(unsigned int)(id_) & 0xFF);

	return str;
}

} // namespace fridge
