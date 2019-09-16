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

#include "fridge/console.h"

#include <Arduino.h>
#include <FS.h>
#ifdef ARDUINO_ARCH_ESP8266
# include <ESP8266WiFi.h>
#else
# include <WiFi.h>
#endif
#include <time.h>

#include <memory>
#include <string>
#include <vector>

#include <uuid/console.h>
#include <uuid/log.h>

#include "fridge/config.h"
#include "fridge/fridge.h"
#include "fridge/network.h"

using ::uuid::flash_string_vector;
using ::uuid::console::Commands;
using ::uuid::console::Shell;
using LogLevel = ::uuid::log::Level;
using LogFacility = ::uuid::log::Facility;

#define MAKE_PSTR(string_name, string_literal) static const char __pstr__##string_name[] __attribute__((__aligned__(sizeof(int)))) PROGMEM = string_literal;
#define MAKE_PSTR_WORD(string_name) MAKE_PSTR(string_name, #string_name)
#define F_(string_name) FPSTR(__pstr__##string_name)

namespace fridge {

MAKE_PSTR_WORD(auto)
MAKE_PSTR_WORD(connect)
MAKE_PSTR_WORD(console)
MAKE_PSTR_WORD(delete)
MAKE_PSTR_WORD(disconnect)
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
MAKE_PSTR_WORD(mark)
MAKE_PSTR_WORD(maximum)
MAKE_PSTR_WORD(memory)
MAKE_PSTR_WORD(mkfs)
MAKE_PSTR_WORD(name)
MAKE_PSTR_WORD(network)
MAKE_PSTR_WORD(off)
MAKE_PSTR_WORD(on)
MAKE_PSTR_WORD(passwd)
MAKE_PSTR_WORD(password)
MAKE_PSTR_WORD(reconnect)
MAKE_PSTR_WORD(relay)
MAKE_PSTR_WORD(restart)
MAKE_PSTR_WORD(scan)
MAKE_PSTR_WORD(sensor)
MAKE_PSTR_WORD(sensors)
MAKE_PSTR_WORD(set)
MAKE_PSTR_WORD(show)
MAKE_PSTR_WORD(ssid)
MAKE_PSTR_WORD(status)
MAKE_PSTR_WORD(su)
MAKE_PSTR_WORD(sync)
MAKE_PSTR_WORD(syslog)
MAKE_PSTR_WORD(system)
MAKE_PSTR_WORD(type)
MAKE_PSTR_WORD(unknown)
MAKE_PSTR_WORD(uptime)
MAKE_PSTR_WORD(version)
MAKE_PSTR_WORD(wifi)
MAKE_PSTR(asterisks, "********")
MAKE_PSTR(celsius_mandatory, "<°C>")
MAKE_PSTR(host_is_fmt, "Host = %s")
MAKE_PSTR(id_mandatory, "<id>")
MAKE_PSTR(invalid_log_level, "Invalid log level")
MAKE_PSTR(ip_address_optional, "[IP address]")
MAKE_PSTR(log_level_is_fmt, "Log level = %s")
MAKE_PSTR(log_level_optional, "[level]")
MAKE_PSTR(minimum_temperature_fmt, "Minimum temperature = %.2f°C");
MAKE_PSTR(mark_interval_is_fmt, "Mark interval = %lus");
MAKE_PSTR(maximum_temperature_fmt, "Maximum temperature = %.2f°C");
MAKE_PSTR(name_mandatory, "<name>")
MAKE_PSTR(name_optional, "[name]")
MAKE_PSTR(new_password_prompt1, "Enter new password: ")
MAKE_PSTR(new_password_prompt2, "Retype new password: ")
MAKE_PSTR(password_prompt, "Password: ")
MAKE_PSTR(seconds_optional, "[seconds]")
MAKE_PSTR(unset, "<unset>")
MAKE_PSTR(wifi_ssid_fmt, "WiFi SSID = %s");
MAKE_PSTR(wifi_password_fmt, "WiFi Password = %S");

static constexpr unsigned long INVALID_PASSWORD_DELAY_MS = 3000;

static void setup_commands(std::shared_ptr<Commands> &commands) {
	#define NO_ARGUMENTS std::vector<std::string>{}

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(console), F_(log)}, flash_string_vector{F_(log_level_optional)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		if (!arguments.empty()) {
			uuid::log::Level level;

			if (uuid::log::parse_level_lowercase(arguments[0], level)) {
				shell.log_level(level);
			} else {
				shell.printfln(F_(invalid_log_level));
				return;
			}
		}
		shell.printfln(F_(log_level_is_fmt), uuid::log::format_level_uppercase(shell.log_level()));
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) -> std::vector<std::string> {
		return uuid::log::levels_lowercase();
	});

	auto main_exit_user_function = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.stop();
	};

	auto main_exit_admin_function = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.logger().log(LogLevel::INFO, LogFacility::AUTH, "Admin session closed on console %s", dynamic_cast<FridgeShell&>(shell).console_name().c_str());
		shell.remove_flags(CommandFlags::ADMIN);
	};

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(exit)},
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		if (shell.has_flags(CommandFlags::ADMIN)) {
			main_exit_admin_function(shell, NO_ARGUMENTS);
		} else {
			main_exit_user_function(shell, NO_ARGUMENTS);
		}
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(help)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.print_all_available_commands();
	});

	auto main_logout_function = [=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		if (shell.has_flags(CommandFlags::ADMIN)) {
			main_exit_admin_function(shell, NO_ARGUMENTS);
		}
		main_exit_user_function(shell, NO_ARGUMENTS);
	};

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(logout)}, main_logout_function);

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(mkfs)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		if (SPIFFS.begin()) {
			shell.logger().warning("Formatting SPIFFS filesystem");
			if (SPIFFS.format()) {
				auto msg = F("Formatted SPIFFS filesystem");
				shell.logger().warning(msg);
				shell.println(msg);
			} else {
				auto msg = F("Error formatting SPIFFS filesystem");
				shell.logger().emerg(msg);
				shell.println(msg);
			}
		} else {
			auto msg = F("Unable to mount SPIFFS filesystem");
			shell.logger().alert(msg);
			shell.println(msg);
		}
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(passwd)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.enter_password(F_(new_password_prompt1),
				[] (Shell &shell, bool completed, const std::string &password1) {
			if (completed) {
				shell.enter_password(F_(new_password_prompt2),
						[password1] (Shell &shell, bool completed, const std::string &password2) {
					if (completed) {
						if (password1 == password2) {
							Config config;
							config.admin_password(password2);
							config.commit();
							shell.println(F("Admin password updated"));
						} else {
							shell.println(F("Passwords do not match"));
						}
					}
				});
			}
		});
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(on)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		Fridge::relay(true);
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(off)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		Fridge::relay(false);
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(relay), F_(auto)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(restart)},
		[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
			ESP.restart();
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(set)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		Config config;
		shell.printfln(F_(minimum_temperature_fmt), config.minimum_temperature());
		shell.printfln(F_(maximum_temperature_fmt), config.maximum_temperature());
		if (shell.has_flags(CommandFlags::ADMIN | CommandFlags::LOCAL)) {
			shell.printfln(F_(wifi_ssid_fmt), config.wifi_ssid().empty() ? uuid::read_flash_string(F_(unset)).c_str() : config.wifi_ssid().c_str());
			shell.printfln(F_(wifi_password_fmt), config.wifi_password().empty() ? F_(unset) : F_(asterisks));
		}
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(set), F_(hostname)}, flash_string_vector{F_(name_optional)},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		Config config;

		if (arguments.empty()) {
			config.hostname("");
		} else {
			config.hostname(arguments.front());
		}
		config.commit();

		Fridge::config_syslog();
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

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(set), F_(wifi), F_(ssid)}, flash_string_vector{F_(name_mandatory)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		Config config;
		config.wifi_ssid(arguments.front());
		config.commit();
		shell.printfln(F_(wifi_ssid_fmt), config.wifi_ssid().empty() ? uuid::read_flash_string(F_(unset)).c_str() : config.wifi_ssid().c_str());
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) -> std::vector<std::string> {
		Config config;
		return std::vector<std::string>{config.wifi_ssid()};
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(set), F_(wifi), F_(password)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.enter_password(F_(new_password_prompt1), [] (Shell &shell, bool completed, const std::string &password1) {
						if (completed) {
							shell.enter_password(F_(new_password_prompt2), [password1] (Shell &shell, bool completed, const std::string &password2) {
								if (completed) {
									if (password1 == password2) {
										Config config;
										config.wifi_password(password2);
										config.commit();
										shell.println(F("WiFi password updated"));
									} else {
										shell.println(F("Passwords do not match"));
									}
								}
							});
						}
					});
	});

	auto show_memory = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.printfln(F("Free heap:                %lu bytes"), (unsigned long)ESP.getFreeHeap());
		shell.printfln(F("Maximum free block size:  %lu bytes"), (unsigned long)ESP.getMaxFreeBlockSize());
		shell.printfln(F("Heap fragmentation:       %u%%"), ESP.getHeapFragmentation());
		shell.printfln(F("Free continuations stack: %lu bytes"), (unsigned long)ESP.getFreeContStack());
	};
	auto show_network = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		Network::print_status(shell);
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
		shell.print(uuid::log::format_timestamp_ms(uuid::get_uptime_ms(), 3));
		shell.println();

		struct timeval tv;
		// time() does not return UTC on the ESP8266: https://github.com/esp8266/Arduino/issues/4637
		if (gettimeofday(&tv, nullptr) == 0) {
			struct tm tm;

			tm.tm_year = 0;
			gmtime_r(&tv.tv_sec, &tm);

			if (tm.tm_year != 0) {
				shell.printfln(F("Time: %04u-%02u-%02uT%02u:%02u:%02u.%06luZ"),
						tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
						tm.tm_hour, tm.tm_min, tm.tm_sec, (unsigned long)tv.tv_usec);
			}
		}
	};
	auto show_version = [] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.println(F("Version: " FRIDGE_REVISION));
	};

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show)},
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		show_memory(shell, NO_ARGUMENTS);
		shell.println();
		show_network(shell, NO_ARGUMENTS);
		shell.println();
		show_relay(shell, NO_ARGUMENTS);
		shell.println();
		show_sensors(shell, NO_ARGUMENTS);
		shell.println();
		show_system(shell, NO_ARGUMENTS);
		shell.println();
		show_uptime(shell, NO_ARGUMENTS);
		shell.println();
		show_version(shell, NO_ARGUMENTS);
	});
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(memory)}, show_relay);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(network)}, show_network);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(relay)}, show_relay);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(sensors)}, show_sensors);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(system)}, show_relay);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(uptime)}, show_relay);
	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(show), F_(version)}, show_relay);

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(su)},
			[=] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		auto become_admin = [] (Shell &shell) {
			shell.logger().log(LogLevel::NOTICE, LogFacility::AUTH, F("Admin session opened on console %s"), dynamic_cast<FridgeShell&>(shell).console_name().c_str());
			shell.add_flags(CommandFlags::ADMIN);
		};

		if (shell.has_flags(CommandFlags::LOCAL)) {
			become_admin(shell);
		} else {
			shell.enter_password(F_(password_prompt), [=] (Shell &shell, bool completed, const std::string &password) {
				if (completed) {
					uint64_t now = uuid::get_uptime_ms();

					if (!password.empty() && password == Config().admin_password()) {
						become_admin(shell);
					} else {
						shell.delay_until(now + INVALID_PASSWORD_DELAY_MS, [] (Shell &shell) {
							shell.logger().log(LogLevel::NOTICE, LogFacility::AUTH, F("Invalid admin password on console %s"), dynamic_cast<FridgeShell&>(shell).console_name().c_str());
							shell.println(F("su: incorrect password"));
						});
					}
				}
			});
		}
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::USER, flash_string_vector{F_(sensor)}, flash_string_vector{F_(id_mandatory)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		dynamic_cast<FridgeShell&>(shell).enter_sensor_context(arguments.front());
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) -> const std::vector<std::string> {
		return std::vector<std::string>{"aaa", "bbb", "ccc"};
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
		main_logout_function(shell, NO_ARGUMENTS);
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

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(sync)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		auto msg = F("Unable to mount SPIFFS filesystem");
		if (SPIFFS.begin()) {
			SPIFFS.end();
			if (!SPIFFS.begin()) {
				shell.logger().alert(msg);
			}
		} else {
			shell.logger().alert(msg);
		}
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(host)}, flash_string_vector{F_(ip_address_optional)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		Config config;
		if (!arguments.empty()) {
			config.syslog_host(arguments[0]);
			config.commit();
		}
		auto host = config.syslog_host();
		shell.printfln(F_(host_is_fmt), !host.empty() ? host.c_str() : uuid::read_flash_string(F_(unset)).c_str());
		Fridge::config_syslog();
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(level)}, flash_string_vector{F_(log_level_optional)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		Config config;
		if (!arguments.empty()) {
			uuid::log::Level level;

			if (uuid::log::parse_level_lowercase(arguments[0], level)) {
				config.syslog_level(level);
				config.commit();
				Fridge::config_syslog();
			} else {
				shell.printfln(F_(invalid_log_level));
				return;
			}
		}
		shell.printfln(F_(log_level_is_fmt), uuid::log::format_level_uppercase(config.syslog_level()));
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) -> std::vector<std::string> {
		return uuid::log::levels_lowercase();
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN, flash_string_vector{F_(syslog), F_(mark)}, flash_string_vector{F_(seconds_optional)},
			[] (Shell &shell, const std::vector<std::string> &arguments) {
		Config config;
		if (!arguments.empty()) {
			config.syslog_mark_interval(String(arguments[0].c_str()).toInt());
			config.commit();
		}
		shell.printfln(F_(mark_interval_is_fmt), config.syslog_mark_interval());
		Fridge::config_syslog();
	});


	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(wifi), F_(connect)},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		Network::connect();
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(wifi), F_(disconnect)},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		Network::disconnect();
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(wifi), F_(reconnect)},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		Network::reconnect();
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(wifi), F_(scan)},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		Network::scan(shell);
	});

	commands->add_command(ShellContext::MAIN, CommandFlags::ADMIN | CommandFlags::LOCAL, flash_string_vector{F_(wifi), F_(status)},
			[&] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		Network::print_status(shell);
	});
}

std::shared_ptr<Commands> FridgeShell::commands_ = [] {
	std::shared_ptr<Commands> commands = std::make_shared<Commands>();
	setup_commands(commands);
	return commands;
} ();

FridgeShell::FridgeShell() : Shell() {

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
	return Shell::exit_context();
}

void FridgeShell::started() {
	logger().log(LogLevel::INFO, LogFacility::CONSOLE, F("User session opened on console %s"), console_name().c_str());
}

void FridgeShell::stopped() {
	if (has_flags(CommandFlags::ADMIN)) {
		logger().log(LogLevel::INFO, LogFacility::AUTH, F("Admin session closed on console %s"), console_name().c_str());
	}
	logger().log(LogLevel::INFO, LogFacility::CONSOLE, F("User session closed on console %s"), console_name().c_str());
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
	std::string hostname = config.hostname();

	if (hostname.empty()) {
		hostname.resize(16, '\0');

		::snprintf_P(&hostname[0], hostname.capacity() + 1, PSTR("fridge-%08x"), ESP.getChipId());
	}

	return hostname;
}

std::string FridgeShell::context_text() {
	switch (static_cast<ShellContext>(context())) {
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
	if (has_flags(CommandFlags::ADMIN)) {
		return "#";
	} else {
		return "$";
	}
}

void FridgeShell::end_of_transmission() {
	if (context() != ShellContext::MAIN || has_flags(CommandFlags::ADMIN)) {
		invoke_command(uuid::read_flash_string(F_(exit)));
	} else {
		invoke_command(uuid::read_flash_string(F_(logout)));
	}
}

FridgeStreamConsole::FridgeStreamConsole(Stream &stream, bool local)
		: uuid::console::Shell(commands_, ShellContext::MAIN, local ? (CommandFlags::USER | CommandFlags::LOCAL) : CommandFlags::USER),
		  uuid::console::StreamConsole(stream),
		  FridgeShell(),
		  name_(uuid::read_flash_string(F("ttyS0"))) {

}

FridgeStreamConsole::FridgeStreamConsole(Stream &stream, const IPAddress &addr, uint16_t port)
		: uuid::console::Shell(commands_, ShellContext::MAIN, CommandFlags::USER),
		  uuid::console::StreamConsole(stream),
		  FridgeShell() {
	std::vector<char> text(64);

	snprintf_P(text.data(), text.size(), PSTR("pty/[%s]:%u"), uuid::printable_to_string(addr).c_str(), port);

	name_ = text.data();
}

std::string FridgeStreamConsole::console_name() {
	return name_;
}

} // namespace fridge
