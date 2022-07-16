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

/*
 * static_cast<float> is required to stop ArduinoJson from taking the address of
 * a static constexpr value that should always be inlined
 */

#define MCU_APP_CONFIG_DATA \
		MCU_APP_CONFIG_CUSTOM(float, "", minimum_temperature, "_c", static_cast<float>(DEFAULT_MINIMUM_TEMPERATURE_C), true) \
		MCU_APP_CONFIG_CUSTOM(float, "", maximum_temperature, "_c", static_cast<float>(DEFAULT_MAXIMUM_TEMPERATURE_C), true)

public:
	float minimum_temperature() const;
	bool minimum_temperature(float temperature, bool load = false);

	float maximum_temperature() const;
	bool maximum_temperature(float temperature, bool load = false);

private:
	static constexpr float MINIMUM_TEMPERATURE_C = -40.0f;
	static constexpr float MAXIMUM_TEMPERATURE_C = 40.0f;
	static constexpr float DEFAULT_MINIMUM_TEMPERATURE_C = 3.0f;
	static constexpr float DEFAULT_MAXIMUM_TEMPERATURE_C = 5.0f;
	static constexpr float DEFAULT_TEMPERATURE_DIFFERENTIAL_C = 2.0f;

	static float minimum_temperature_;
	static float maximum_temperature_;
