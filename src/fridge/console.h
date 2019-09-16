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

#ifndef FRIDGE_CONSOLE_H_
#define FRIDGE_CONSOLE_H_

#include <uuid/console.h>

#ifdef ARDUINO_ARCH_ESP8266
# include <ESP8266WiFi.h>
#else
# include <WiFi.h>
#endif

#include <memory>
#include <string>

#ifdef LOCAL
# undef LOCAL
#endif

namespace fridge {

enum CommandFlags : unsigned int {
	USER = 0,
	ADMIN = (1 << 0),
	LOCAL = (1 << 1),
};

enum ShellContext : unsigned int {
	MAIN = 0,
	SENSOR,
};

class FridgeShell: virtual public uuid::console::Shell {
public:
	~FridgeShell() override = default;

	virtual std::string console_name() = 0;
	void enter_sensor_context(std::string sensor);
	bool exit_context() override;

protected:
	FridgeShell();

	static std::shared_ptr<uuid::console::Commands> commands_;

	void started() override;
	void display_banner() override;
	std::string hostname_text() override;
	std::string context_text() override;
	std::string prompt_suffix() override;
	void end_of_transmission() override;
	void stopped() override;

private:
	std::string sensor_;
};

class FridgeStreamConsole: public uuid::console::StreamConsole, public FridgeShell {
public:
	FridgeStreamConsole(Stream &stream, bool local);
	FridgeStreamConsole(Stream &stream, const IPAddress &addr, uint16_t port);
	~FridgeStreamConsole() override = default;

	std::string console_name();

private:
	std::string name_;
};

} // namespace fridge

#endif
