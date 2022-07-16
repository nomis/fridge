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

#include "fridge/console.h"

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <uuid/console.h>
#include <uuid/log.h>

#include "fridge/app.h"
#include "app/config.h"
#include "app/console.h"

using ::uuid::flash_string_vector;
using ::uuid::console::Commands;
using ::uuid::console::Shell;
using LogLevel = ::uuid::log::Level;
using LogFacility = ::uuid::log::Facility;

using ::app::AppShell;
using ::app::CommandFlags;
using ::app::Config;
using ::app::ShellContext;

#define MAKE_PSTR(string_name, string_literal) static const char __pstr__##string_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = string_literal;
#define MAKE_PSTR_WORD(string_name) MAKE_PSTR(string_name, #string_name)
#define F_(string_name) FPSTR(__pstr__##string_name)

namespace fridge {

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wunused-const-variable"
MAKE_PSTR_WORD(auto)
MAKE_PSTR_WORD(delete)
MAKE_PSTR_WORD(exit)
MAKE_PSTR_WORD(external)
MAKE_PSTR_WORD(help)
MAKE_PSTR_WORD(internal)
MAKE_PSTR_WORD(logout)
MAKE_PSTR_WORD(minimum)
MAKE_PSTR_WORD(maximum)
MAKE_PSTR_WORD(name)
MAKE_PSTR_WORD(off)
MAKE_PSTR_WORD(on)
MAKE_PSTR_WORD(relay)
MAKE_PSTR_WORD(sensor)
MAKE_PSTR_WORD(sensors)
MAKE_PSTR_WORD(set)
MAKE_PSTR_WORD(show)
MAKE_PSTR_WORD(type)
MAKE_PSTR_WORD(unknown)
MAKE_PSTR(celsius_mandatory, "<°C>")
MAKE_PSTR(id_mandatory, "<id>")
MAKE_PSTR(minimum_temperature_fmt, "Minimum temperature = %.2f°C");
MAKE_PSTR(maximum_temperature_fmt, "Maximum temperature = %.2f°C");
MAKE_PSTR(name_optional, "[name]")
#pragma GCC diagnostic pop

static inline App &to_app(Shell &shell) {
	return static_cast<App&>(dynamic_cast<app::AppShell&>(shell).app_);
}

static inline FridgeShell &to_shell(Shell &shell) {
	return dynamic_cast<FridgeShell&>(shell);
}

#define NO_ARGUMENTS std::vector<std::string>{}

static inline void setup_commands(std::shared_ptr<Commands> &commands) {
	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(on)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		to_app(shell).relay(true);
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(off)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		to_app(shell).relay(false);
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(auto)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(minimum)}, flash_string_vector{F_(celsius_mandatory)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		Config config;
		bool max_changed = config.minimum_temperature(String(arguments.front().c_str()).toFloat());
		config.commit();

		shell.printfln(F_(minimum_temperature_fmt), config.minimum_temperature());
		if (max_changed) {
			shell.printfln(F_(maximum_temperature_fmt), config.maximum_temperature());
		}
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(maximum)}, flash_string_vector{F_(celsius_mandatory)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		Config config;
		bool min_changed = config.maximum_temperature(String(arguments.front().c_str()).toFloat());
		config.commit();

		if (min_changed) {
			shell.printfln(F_(minimum_temperature_fmt), config.minimum_temperature());
		}
		shell.printfln(F_(maximum_temperature_fmt), config.maximum_temperature());
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(relay)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(sensors)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		for (auto& device : to_app(shell).sensor_devices()) {
			shell.printfln(F("Sensor %s: %.2fC"), device.to_string().c_str(), device.temperature_c_);
		}
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(sensor)}, flash_string_vector{F_(id_mandatory)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		to_shell(shell).enter_sensor_context(arguments.front());
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) -> const std::vector<std::string> {
		std::vector<std::string> devices;

		for (auto& device : to_app(shell).sensor_devices()) {
			devices.emplace_back(device.to_string());
		}

		return devices;
	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(delete)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	auto sensor_exit_function = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.exit_context();
	};

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(exit)}, sensor_exit_function);

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(help)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.print_all_available_commands();
	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(logout)},
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		sensor_exit_function(shell, NO_ARGUMENTS);
		AppShell::main_logout_function(shell, NO_ARGUMENTS);
	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(show)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(set)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(name)}, flash_string_vector{F_(name_optional)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(type), F_(unknown)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(type), F_(internal)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(type), F_(external)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});
}

FridgeShell::FridgeShell(app::App &app) : Shell(), AppShell(app) {

}

void FridgeShell::enter_sensor_context(std::string sensor) {
	if (context() == ShellContext::MAIN) {
		enter_context(ShellContext::SENSOR);
		sensor_ = sensor;
	}
}

bool FridgeShell::exit_context() {
	if (context() == ShellContext::SENSOR) {
		sensor_ = std::string{};
	}
	return AppShell::exit_context();
}

void FridgeShell::display_banner() {
	AppShell::display_banner();
	println(F("┌─────────────────────────────────────────────────────────────────────────┐"));
	println(F("│“I do believe,” said Detritius, “that I am genuinely cogitating. How very│"));
	println(F("│interesting!” .... More ice cascaded off Detritus as he rubbed his head. │"));
	println(F("│“Of course!” he said, holding up a giant finger. “Superconductivity!”    │"));
	println(F("└─────────────────────────────────────────────────────────────────────────┘"));
	println();
}

std::string FridgeShell::context_text() {
	switch (static_cast<ShellContext>(context())) {
	case ShellContext::MAIN:
		return std::string{'/'};

	case ShellContext::SENSOR:
		return uuid::read_flash_string(F("/sensors/")) + sensor_;
	}

	return std::string{};
}

} // namespace fridge

namespace app {

void setup_commands(std::shared_ptr<Commands> &commands) {
	fridge::setup_commands(commands);
}

} // namespace app
