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

#include "app/config.h"

#include <cmath>

namespace app {

bool Config::minimum_temperature(float temperature, bool load) {
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

bool Config::maximum_temperature(float temperature, bool load) {
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

} // namespace app
