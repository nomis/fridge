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

#include "config.h"

#include <Arduino.h>
#include <FS.h>

#include <cmath>
#include <string>
#include <vector>

#include <uuid/common.h>
#include <uuid/log.h>
#include <ArduinoJson.hpp>

#include "main.h"

#define MAKE_PSTR(string_name, string_literal) static const char __pstr__##string_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = string_literal;
#define MAKE_PSTR_WORD(string_name) MAKE_PSTR(string_name, #string_name)
#define F_(string_name) FPSTR(__pstr__##string_name)

namespace fridge {

std::string Config::admin_password_;
std::string Config::hostname_;
float Config::minimum_temperature_;
float Config::maximum_temperature_;
std::string Config::wifi_ssid_;
std::string Config::wifi_password_;

MAKE_PSTR_WORD(admin_password)
MAKE_PSTR_WORD(hostname)
MAKE_PSTR_WORD(minimum_temperature_c)
MAKE_PSTR_WORD(maximum_temperature_c)
MAKE_PSTR_WORD(wifi_ssid)
MAKE_PSTR_WORD(wifi_password)

void Config::read_config(const ArduinoJson::JsonDocument &doc) {
	set_admin_password(doc[F_(admin_password)] | "");
	set_hostname(doc[F_(hostname)] | "");
	set_minimum_temperature(doc[F_(minimum_temperature_c)] | DEFAULT_MINIMUM_TEMPERATURE_C, true);
	set_maximum_temperature(doc[F_(maximum_temperature_c)] | DEFAULT_MAXIMUM_TEMPERATURE_C, true);
	set_wifi_ssid(doc[F_(wifi_ssid)] | "");
	set_wifi_password(doc[F_(wifi_password)] | "");
}

void Config::write_config(ArduinoJson::JsonDocument &doc) {
	doc[F_(admin_password)] = get_hostname().c_str();
	doc[F_(hostname)] = get_hostname().c_str();
	doc[F_(minimum_temperature_c)] = get_minimum_temperature();
	doc[F_(maximum_temperature_c)] = get_maximum_temperature();
	doc[F_(wifi_ssid)] = get_wifi_ssid().c_str();
	doc[F_(wifi_password)] = get_wifi_password().c_str();
}

static const char __pstr__config_filename[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "/config.msgpack";
static const char __pstr__config_backup_filename[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "/config.msgpack~";

static const char __pstr__logger_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = "config";
uuid::log::Logger Config::logger_{FPSTR(__pstr__logger_name), uuid::log::Facility::DAEMON};

bool Config::mounted_ = false;
bool Config::unavailable_ = false;
bool Config::loaded_ = false;

Config::Config() {
	if (!unavailable_ && !mounted_) {
		logger_.info(F("Mounting SPIFFS filesystem"));
		if (SPIFFS.begin()) {
			logger_.info(F("Mounted SPIFFS filesystem"));
			mounted_ = true;
		} else {
			logger_.alert(F("Unable to mount SPIFFS filesystem"));
			unavailable_ = true;
		}
	}

	if (mounted_ && !loaded_) {
		if (read_config(uuid::read_flash_string(FPSTR(__pstr__config_filename)))
				|| read_config(uuid::read_flash_string(FPSTR(__pstr__config_backup_filename)))) {
			loaded_ = true;
		}
	}

	if (!loaded_) {
		logger_.err(F("Config failure, using defaults"));
		read_config(ArduinoJson::StaticJsonDocument<0>());
		loaded_ = true;
	}
}

std::string Config::get_admin_password() {
	return admin_password_;
}

void Config::set_admin_password(const std::string &password) {
	admin_password_ = password;
}

std::string Config::get_hostname() {
	return hostname_;
}

void Config::set_hostname(const std::string &name) {
	hostname_ = name;
}

float Config::get_minimum_temperature() {
	return minimum_temperature_;
}

bool Config::set_minimum_temperature(float temperature, bool load) {
	if (!std::isfinite(temperature)) {
		if (load) {
			temperature = DEFAULT_MINIMUM_TEMPERATURE_C;
		} else {
			return false;
		}
	}

	temperature = std::max(temperature, MINIMUM_TEMPERATURE_C);
	temperature = std::min(temperature, MAXIMUM_TEMPERATURE_C);
	minimum_temperature_ = temperature;

	if (maximum_temperature_ < minimum_temperature_) {
		maximum_temperature_ = minimum_temperature_ + DEFAULT_TEMPERATURE_DIFFERENTIAL_C;
		return true;
	} else {
		return false;
	}
}

float Config::get_maximum_temperature() {
	return maximum_temperature_;
}

bool Config::set_maximum_temperature(float temperature, bool load) {
	if (!std::isfinite(temperature)) {
		if (load) {
			temperature = DEFAULT_MAXIMUM_TEMPERATURE_C;
		} else {
			return false;
		}
	}

	temperature = std::max(temperature, MINIMUM_TEMPERATURE_C);
	temperature = std::min(temperature, MAXIMUM_TEMPERATURE_C);
	maximum_temperature_ = temperature;

	if (std::isfinite(maximum_temperature_) && minimum_temperature_ > maximum_temperature_) {
		minimum_temperature_ = maximum_temperature_ - DEFAULT_TEMPERATURE_DIFFERENTIAL_C;
		return true;
	} else {
		return false;
	}
}

std::string Config::get_wifi_ssid() {
	return wifi_ssid_;
}

void Config::set_wifi_ssid(const std::string &name) {
	wifi_ssid_ = name;
}

std::string Config::get_wifi_password() {
	return wifi_password_;
}

void Config::set_wifi_password(const std::string &password) {
	wifi_password_ = password;
}

void Config::commit() {
	if (mounted_) {
		std::string filename = uuid::read_flash_string(FPSTR(__pstr__config_filename));
		std::string backup_filename = uuid::read_flash_string(FPSTR(__pstr__config_backup_filename));

		if (write_config(filename)) {
			if (read_config(filename, false)) {
				write_config(backup_filename);
			}
		}
	}
}

bool Config::read_config(const std::string &filename, bool load) {
	logger_.info(F("Reading config file %s"), filename.c_str());
	File file = SPIFFS.open(filename.c_str(), "r");
	if (file) {
		ArduinoJson::DynamicJsonDocument doc(BUFFER_SIZE);

		auto error = ArduinoJson::deserializeMsgPack(doc, file);
		if (error) {
			logger_.err(F("Failed to parse config file %s: %s"), filename.c_str(), error.c_str());
			return false;
		} else {
			if (load) {
				logger_.info(F("Loading config from file %s"), filename.c_str());
				read_config(doc);
			}
			return true;
		}
	} else {
		logger_.err(F("Config file %s does not exist"), filename.c_str());
		return false;
	}
}

bool Config::write_config(const std::string &filename) {
	logger_.info(F("Writing config file %s"), filename.c_str());
	File file = SPIFFS.open(filename.c_str(), "w");
	if (file) {
		ArduinoJson::DynamicJsonDocument doc(BUFFER_SIZE);

		write_config(doc);

		ArduinoJson::serializeMsgPack(doc, file);

		if (file.getWriteError()) {
			logger_.alert(F("Failed to write config file %s: %u"), filename.c_str(), file.getWriteError());
			return false;
		} else {
			return true;
		}
	} else {
		logger_.alert(F("Unable to open config file %s for writing"), filename.c_str());
		return false;
	}
}

} // namespace fridge
