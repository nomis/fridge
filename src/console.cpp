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

#include "console.h"

#include <Arduino.h>
#include <FS.h>

#include <string>

#include <uuid/console.h>
#include <uuid/log.h>

#include "config.h"
#include "main.h"

using ::uuid::flash_string_vector;
using ::uuid::console::Commands;
using ::uuid::console::Shell;
using ::uuid::console::StreamConsole;
using LogLevel = ::uuid::log::Level;

#define MAKE_PSTR(string_name, string_literal) static const char __pstr__##string_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = string_literal;
#define MAKE_PSTR_WORD(string_name) MAKE_PSTR(string_name, #string_name)
#define F_(string_name) FPSTR(__pstr__##string_name)

MAKE_PSTR_WORD(auto)
MAKE_PSTR_WORD(console)
MAKE_PSTR_WORD(delete)
MAKE_PSTR_WORD(exit)
MAKE_PSTR_WORD(external)
MAKE_PSTR_WORD(help)
MAKE_PSTR_WORD(host)
MAKE_PSTR_WORD(hostname)
MAKE_PSTR_WORD(internal)
MAKE_PSTR_WORD(level)
MAKE_PSTR_WORD(log)
MAKE_PSTR_WORD(logout)
MAKE_PSTR_WORD(minimum)
MAKE_PSTR_WORD(maximum)
MAKE_PSTR_WORD(mkfs)
MAKE_PSTR_WORD(memory)
MAKE_PSTR_WORD(name)
MAKE_PSTR_WORD(network)
MAKE_PSTR_WORD(off)
MAKE_PSTR_WORD(on)
MAKE_PSTR_WORD(passwd)
MAKE_PSTR_WORD(password)
MAKE_PSTR_WORD(relay)
MAKE_PSTR_WORD(restart)
MAKE_PSTR_WORD(sensor)
MAKE_PSTR_WORD(sensors)
MAKE_PSTR_WORD(set)
MAKE_PSTR_WORD(show)
MAKE_PSTR_WORD(ssid)
MAKE_PSTR_WORD(su)
MAKE_PSTR_WORD(sync)
MAKE_PSTR_WORD(syslog)
MAKE_PSTR_WORD(system)
MAKE_PSTR_WORD(trace)
MAKE_PSTR_WORD(type)
MAKE_PSTR_WORD(unknown)
MAKE_PSTR_WORD(uptime)
MAKE_PSTR_WORD(version)
MAKE_PSTR_WORD(wifi)
MAKE_PSTR(asterisks, "********")
MAKE_PSTR(celsius_mandatory, "<°C>")
MAKE_PSTR(id_mandatory, "<id>")
MAKE_PSTR(invalid_password, "su: incorrect password")
MAKE_PSTR(ip_address_optional, "[IP address]")
MAKE_PSTR(log_level_is_fmt, "Log level = %s")
MAKE_PSTR(minimum_temperature_fmt, "Minimum temperature = %.2f°C");
MAKE_PSTR(maximum_temperature_fmt, "Maximum temperature = %.2f°C");
MAKE_PSTR(name_mandatory, "<name>")
MAKE_PSTR(name_optional, "[name]")
MAKE_PSTR(new_password_prompt1, "Enter new password: ")
MAKE_PSTR(new_password_prompt2, "Retype new password: ")
MAKE_PSTR(password_prompt, "Password: ")
MAKE_PSTR(unset, "<unset>")
MAKE_PSTR(wifi_ssid_fmt, "WiFi SSID = %s");
MAKE_PSTR(wifi_password_fmt, "WiFi Password = %S");

static constexpr unsigned long INVALID_PASSWORD_DELAY_MS = 3000;

static void add_console_log_command(std::shared_ptr<Commands> &commands, LogLevel level) {
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(console), F_(log), uuid::log::format_level_lowercase(level)}, Commands::no_arguments,
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.set_log_level(level);
		shell.printfln(F_(log_level_is_fmt), uuid::log::format_level_uppercase(shell.get_log_level()));
	}, Commands::no_argument_completion);
}

static void add_syslog_level_command(std::shared_ptr<Commands> &commands, LogLevel level) {
	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(level), uuid::log::format_level_lowercase(level)}, Commands::no_arguments,
			[=] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);
}

static void setup_commands(std::shared_ptr<Commands> &commands) {
	const std::vector<std::string> no_arguments{};

	auto console_log = [] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.printfln(F_(log_level_is_fmt), uuid::log::format_level_uppercase(shell.get_log_level()));
	};

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(console), F_(log)}, Commands::no_arguments, console_log, Commands::no_argument_completion);

	add_console_log_command(commands, LogLevel::OFF);
	add_console_log_command(commands, LogLevel::EMERG);
	add_console_log_command(commands, LogLevel::CRIT);
	add_console_log_command(commands, LogLevel::ALERT);
	add_console_log_command(commands, LogLevel::ERR);
	add_console_log_command(commands, LogLevel::WARNING);
	add_console_log_command(commands, LogLevel::NOTICE);
	add_console_log_command(commands, LogLevel::INFO);
	add_console_log_command(commands, LogLevel::DEBUG);
	add_console_log_command(commands, LogLevel::TRACE);
	add_console_log_command(commands, LogLevel::ALL);

	auto main_exit_user_function = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		dynamic_cast<FridgeShell&>(shell).stop();
	};

	auto main_exit_admin_function = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		Shell::logger_.log(uuid::log::Level::INFO, uuid::log::Facility::AUTH, "Admin session closed on console %s", dynamic_cast<FridgeShell&>(shell).console_name().c_str());
		dynamic_cast<FridgeShell&>(shell).flags_ &= ~CommandFlags::ADMIN;
	};

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(exit)}, Commands::no_arguments,
			[=] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		if (shell.flags_ & CommandFlags::ADMIN) {
			main_exit_admin_function(shell, no_arguments);
		} else {
			main_exit_user_function(shell, no_arguments);
		}
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(help)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(on)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		digitalWrite(RELAY_PIN, HIGH);
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(off)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		digitalWrite(RELAY_PIN, LOW);
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(auto)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(mkfs)}, Commands::no_arguments,
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		if (SPIFFS.begin()) {
			shell.logger_.warning("Formatting SPIFFS filesystem");
			if (SPIFFS.format()) {
				auto msg = F("Formatted SPIFFS filesystem");
				shell.logger_.warning(msg);
				shell.println(msg);
			} else {
				auto msg = F("Error formatting SPIFFS filesystem");
				shell.logger_.emerg(msg);
				shell.println(msg);
			}
		} else {
			auto msg = F("Unable to mount SPIFFS filesystem");
			shell.logger_.alert(msg);
			shell.println(msg);
		}

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(passwd)}, Commands::no_arguments,
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.enter_password(F_(new_password_prompt1), [] (Shell &shell __attribute__((unused)), bool completed, const std::string &password1) {
						if (completed) {
							shell.enter_password(F_(new_password_prompt2), [password1] (Shell &shell __attribute__((unused)), bool completed, const std::string &password2) {
								if (completed) {
									if (password1 == password2) {
										Config config;
										config.set_admin_password(password2);
										config.commit();
										shell.println(F("Admin password updated"));
									} else {
										shell.println(F("Passwords do not match"));
									}
								}
							});
						}
					});
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(set)}, Commands::no_arguments,
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		Config config;
		shell.printfln(F_(minimum_temperature_fmt), config.get_minimum_temperature());
		shell.printfln(F_(maximum_temperature_fmt), config.get_maximum_temperature());
		if ((shell.flags_ & CommandFlags::ADMIN) && (shell.flags_ & CommandFlags::LOCAL)) {
			shell.printfln(F_(wifi_ssid_fmt), config.get_wifi_ssid().empty() ? uuid::read_flash_string(F_(unset)).c_str() : config.get_wifi_ssid().c_str());
			shell.printfln(F_(wifi_password_fmt), config.get_wifi_password().empty() ? F_(unset) : F_(asterisks));
		}
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(hostname)}, flash_string_vector{F_(name_optional)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		Config config;

		if (arguments.empty()) {
			config.set_hostname("");
		} else {
			config.set_hostname(arguments.front());
		}
		config.commit();
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(minimum)}, flash_string_vector{F_(celsius_mandatory)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		Config config;
		bool max_changed = config.set_minimum_temperature(String(arguments.front().c_str()).toFloat());
		config.commit();

		shell.printfln(F_(minimum_temperature_fmt), config.get_minimum_temperature());
		if (max_changed) {
			shell.printfln(F_(maximum_temperature_fmt), config.get_maximum_temperature());
		}
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(maximum)}, flash_string_vector{F_(celsius_mandatory)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		Config config;
		bool min_changed = config.set_maximum_temperature(String(arguments.front().c_str()).toFloat());
		config.commit();

		if (min_changed) {
			shell.printfln(F_(minimum_temperature_fmt), config.get_minimum_temperature());
		}
		shell.printfln(F_(maximum_temperature_fmt), config.get_maximum_temperature());
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(set), F_(wifi), F_(ssid)}, flash_string_vector{F_(name_mandatory)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		Config config;
		config.set_wifi_ssid(arguments.front());
		config.commit();
		shell.printfln(F_(wifi_ssid_fmt), config.get_wifi_ssid().empty() ? uuid::read_flash_string(F_(unset)).c_str() : config.get_wifi_ssid().c_str());
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(set), F_(wifi), F_(password)}, Commands::no_arguments,
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.enter_password(F_(new_password_prompt1), [] (Shell &shell __attribute__((unused)), bool completed, const std::string &password1) {
						if (completed) {
							shell.enter_password(F_(new_password_prompt2), [password1] (Shell &shell __attribute__((unused)), bool completed, const std::string &password2) {
								if (completed) {
									if (password1 == password2) {
										Config config;
										config.set_wifi_password(password2);
										config.commit();
										shell.println(F("WiFi password updated"));
									} else {
										shell.println(F("Passwords do not match"));
									}
								}
							});
						}
					});
	}, Commands::no_argument_completion);

	auto show_memory = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.printfln(F("Free heap:                %lu bytes"), (unsigned long)ESP.getFreeHeap());
		shell.printfln(F("Maximum free block size:  %lu bytes"), (unsigned long)ESP.getMaxFreeBlockSize());
		shell.printfln(F("Heap fragmentation:       %u%%"), ESP.getHeapFragmentation());
		shell.printfln(F("Free continuations stack: %lu bytes"), (unsigned long)ESP.getFreeContStack());
	};
	auto show_network = [] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	};
	auto show_relay = [] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	};
	auto show_sensors = [] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	};
	auto show_system = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.printfln(F("Chip ID:       0x%08x"), ESP.getChipId());
		shell.printfln(F("SDK version:   %s"), ESP.getSdkVersion());
		shell.printfln(F("Core version:  %s"), ESP.getCoreVersion().c_str());
		shell.printfln(F("Full version:  %s"), ESP.getFullVersion().c_str());
		shell.printfln(F("Boot version:  %u"), ESP.getBootVersion());
		shell.printfln(F("Boot mode:     %u"), ESP.getBootMode());
		shell.printfln(F("CPU frequency: %u MHz"), ESP.getCpuFreqMHz());
		shell.printfln(F("Flash chip:    0x%08X (%u bytes)"), ESP.getFlashChipId(), ESP.getFlashChipRealSize());
		shell.printfln(F("Sketch size:   %u bytes (%u bytes free)"), ESP.getSketchSize(), ESP.getFreeSketchSpace());
		shell.printfln(F("Reset reason:  %s"), ESP.getResetReason().c_str());
		shell.printfln(F("Reset info:    %s"), ESP.getResetInfo().c_str());

		FSInfo info;
		if (SPIFFS.info(info)) {
			shell.printfln(F("SPIFFS size:   %zu bytes (block size %zu bytes, page size %zu bytes)"), info.totalBytes, info.blockSize, info.pageSize);
			shell.printfln(F("SPIFFS used:   %zu bytes (%.2f%%)"), info.usedBytes, (float)info.usedBytes / (float)info.totalBytes);
		}
	};
	auto show_uptime = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.print(F("Uptime: "));
		shell.print(uuid::log::format_timestamp_ms(3, uuid::get_uptime_ms()));
		shell.println();
	};
	auto show_version = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.println(F("Version: " FRIDGE_REVISION));
	};

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show)}, Commands::no_arguments,
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		show_memory(shell, no_arguments);
		shell.println();
		show_network(shell, no_arguments);
		shell.println();
		show_relay(shell, no_arguments);
		shell.println();
		show_sensors(shell, no_arguments);
		shell.println();
		show_system(shell, no_arguments);
		shell.println();
		show_uptime(shell, no_arguments);
		shell.println();
		show_version(shell, no_arguments);
	}, Commands::no_argument_completion);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(memory)}, Commands::no_arguments, show_relay, Commands::no_argument_completion);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(network)}, Commands::no_arguments, show_network, Commands::no_argument_completion);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(relay)}, Commands::no_arguments, show_relay, Commands::no_argument_completion);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(sensors)}, Commands::no_arguments, show_sensors, Commands::no_argument_completion);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(system)}, Commands::no_arguments, show_relay, Commands::no_argument_completion);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(uptime)}, Commands::no_arguments, show_relay, Commands::no_argument_completion);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(version)}, Commands::no_arguments, show_relay, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(su)}, Commands::no_arguments,
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		auto become_admin = [] (Shell &shell) {
			Shell::logger_.log(uuid::log::Level::NOTICE, uuid::log::Facility::AUTH, "Admin session opened on console %s", dynamic_cast<FridgeShell&>(shell).console_name().c_str());
			shell.flags_ |= CommandFlags::ADMIN;
		};

		if (shell.flags_ & CommandFlags::LOCAL) {
			become_admin(shell);
		} else {
			shell.enter_password(F_(password_prompt), [=] (Shell &shell, bool completed, const std::string &password) {
				if (completed) {
					uint64_t now = uuid::get_uptime_ms();

					if (!password.empty() && password == Config().get_admin_password()) {
						become_admin(shell);
					} else {
						shell.delay_until(now + INVALID_PASSWORD_DELAY_MS, [] (Shell &shell) {
							Shell::logger_.log(uuid::log::Level::NOTICE, uuid::log::Facility::AUTH, "Invalid admin password on console %s", dynamic_cast<FridgeShell&>(shell).console_name().c_str());
							shell.println(F_(invalid_password));
						});
					}
				}
			});
		}
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(sync)}, Commands::no_arguments,
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		auto msg = F("Unable to mount SPIFFS filesystem");
		if (SPIFFS.begin()) {
			SPIFFS.end();
			if (!SPIFFS.begin()) {
				shell.logger_.alert(msg);
			}
		} else {
			shell.logger_.alert(msg);
		}

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(restart)}, Commands::no_arguments,
		[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
			ESP.restart();
	}, Commands::no_argument_completion);

	auto main_logout_function = [=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		if (shell.flags_ & CommandFlags::ADMIN) {
			main_exit_admin_function(shell, no_arguments);
		}
		main_exit_user_function(shell, no_arguments);
	};

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(logout)}, Commands::no_arguments, main_logout_function, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(sensor)}, flash_string_vector{F_(id_mandatory)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		FridgeShell &fridge_shell = dynamic_cast<FridgeShell&>(shell);

		fridge_shell.context_ = ShellContext::SENSOR;
		fridge_shell.sensor_ = arguments.front();
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) -> const std::set<std::string> {
		return std::set<std::string>{"aaa", "bbb", "ccc"};
	});

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(delete)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(show)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(set)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(name)}, flash_string_vector{F_(name_optional)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(type), F_(unknown)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(type), F_(internal)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::SENSOR, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(type), F_(external)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	auto sensor_exit_function = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		dynamic_cast<FridgeShell&>(shell).context_ = ShellContext::MAIN;
	};

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(exit)}, Commands::no_arguments, sensor_exit_function, Commands::no_argument_completion);

	commands->add_command(ShellContext::SENSOR, CommandFlags::USER, flash_string_vector{F_(logout)}, Commands::no_arguments,
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		sensor_exit_function(shell, no_arguments);
		main_logout_function(shell, no_arguments);
	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(host)}, flash_string_vector{F_(ip_address_optional)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(level)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	add_syslog_level_command(commands, LogLevel::OFF);
	add_syslog_level_command(commands, LogLevel::EMERG);
	add_syslog_level_command(commands, LogLevel::CRIT);
	add_syslog_level_command(commands, LogLevel::ALERT);
	add_syslog_level_command(commands, LogLevel::ERR);
	add_syslog_level_command(commands, LogLevel::WARNING);
	add_syslog_level_command(commands, LogLevel::NOTICE);
	add_syslog_level_command(commands, LogLevel::INFO);
	add_syslog_level_command(commands, LogLevel::DEBUG);
	add_syslog_level_command(commands, LogLevel::TRACE);
	add_syslog_level_command(commands, LogLevel::ALL);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(level), F_(log), F_(trace)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(level), F_(log), F_(off)}, Commands::no_arguments,
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	}, Commands::no_argument_completion);
}

std::shared_ptr<Commands> FridgeShell::commands_ = [] {
	std::shared_ptr<Commands> commands = std::make_shared<Commands>();
	setup_commands(commands);
	return commands;
} ();

FridgeShell::FridgeShell() : Shell() {

}

void FridgeShell::started() {
	logger_.log(uuid::log::Level::INFO, uuid::log::Facility::CONSOLE, "User session opened on console %s", console_name().c_str());
}

void FridgeShell::stopped() {
	logger_.log(uuid::log::Level::INFO, uuid::log::Facility::CONSOLE, "User session closed on console %s", console_name().c_str());
}

void FridgeShell::display_banner() {
	printfln(F("fridge " FRIDGE_REVISION));
	println();
	println(F("┌─────────────────────────────────────────────────────────────────────────┐"));
	println(F("│“I do believe,” said Detritius, “that I am genuinely cogitating. How very│"));
	println(F("│interesting!” .... More ice cascaded off Detritus as he rubbed his head. │"));
	println(F("│“Of course!” he said, holding up a giant finger. “Superconductivity!”    │"));
	println(F("└─────────────────────────────────────────────────────────────────────────┘"));
	println();
}

std::string FridgeShell::hostname_text() {
	Config config;
	std::string hostname = config.get_hostname();

	if (hostname.empty()) {
		hostname.resize(16, '\0');

		::snprintf_P(&hostname[0], hostname.capacity() + 1, PSTR("fridge-%08x"), ESP.getChipId());
	}

	return hostname;
}

std::string FridgeShell::context_text() {
	switch (static_cast<ShellContext>(context_)) {
	case ShellContext::MAIN:
		return "/";
		break;

	case ShellContext::SENSOR:
		return sensor_;
		break;
	}

	return "";
}

std::string FridgeShell::prompt_suffix() {
	if (flags_ & CommandFlags::ADMIN) {
		return "#";
	} else {
		return "$";
	}
}

void FridgeShell::end_of_transmission() {
	if (context_ != ShellContext::MAIN || (flags_ & CommandFlags::ADMIN)) {
		invoke_command(uuid::read_flash_string(F_(exit)));
	} else {
		invoke_command(uuid::read_flash_string(F_(logout)));
	}
}

FridgeStreamConsole::FridgeStreamConsole(Stream &stream, bool local)
		: uuid::console::Shell(commands_, ShellContext::MAIN, local ? (CommandFlags::USER | CommandFlags::LOCAL) : CommandFlags::USER), uuid::console::StreamConsole(stream), FridgeShell() {

}

std::string FridgeStreamConsole::console_name() {
	return uuid::read_flash_string(F("ttyS0"));
}
