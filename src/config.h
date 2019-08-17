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

#ifndef FRIDGE_SRC_CONFIG_H_
#define FRIDGE_SRC_CONFIG_H_

#include <ArduinoJson.hpp>

#include <string>

#include <uuid/log.h>

namespace fridge {

class Config {
public:
	Config();
	~Config() = default;

	std::string get_admin_password() const;
	void set_admin_password(const std::string &admin_password);

	std::string get_hostname() const;
	void set_hostname(const std::string &hostname);

	float get_minimum_temperature() const;
	bool set_minimum_temperature(float temperature, bool load = false);

	float get_maximum_temperature() const;
	bool set_maximum_temperature(float temperature, bool load = false);

	std::string get_wifi_ssid() const;
	void set_wifi_ssid(const std::string &wifi_ssid);

	std::string get_wifi_password() const;
	void set_wifi_password(const std::string &wifi_password);

	void commit();

private:
	static constexpr size_t BUFFER_SIZE = 4096;
	static constexpr float MINIMUM_TEMPERATURE_C = -40.0f;
	static constexpr float MAXIMUM_TEMPERATURE_C = 40.0f;
	static constexpr float DEFAULT_MINIMUM_TEMPERATURE_C = 3.0f;
	static constexpr float DEFAULT_MAXIMUM_TEMPERATURE_C = 5.0f;
	static constexpr float DEFAULT_TEMPERATURE_DIFFERENTIAL_C = 2.0f;

	static uuid::log::Logger logger_;

	static bool mounted_;
	static bool unavailable_;
	static bool loaded_;

	static std::string admin_password_;
	static std::string hostname_;
	static float minimum_temperature_;
	static float maximum_temperature_;
	static std::string wifi_password_;
	static std::string wifi_ssid_;

	bool read_config(const std::string &filename, bool load = true);
	void read_config(const ArduinoJson::JsonDocument &doc);
	bool write_config(const std::string &filename);
	void write_config(ArduinoJson::JsonDocument &doc);
};

} // namespace fridge

#endif
