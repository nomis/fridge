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

#include "app/console.h"

#include <memory>
#include <string>
#include <vector>

namespace fridge {

class FridgeShell: public app::AppShell {
public:
	~FridgeShell() override = default;

	void enter_sensor_context(std::string sensor);
	bool exit_context() override;

protected:
	FridgeShell(app::App &app);

	void display_banner() override;
	std::string context_text() override;

private:
	std::string sensor_;
};

} // namespace fridge
