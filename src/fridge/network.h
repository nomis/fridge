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

#ifndef FRIDGE_NETWORK_H_
#define FRIDGE_NETWORK_H_

#include <Arduino.h>
#ifdef ARDUINO_ARCH_ESP8266
# include <ESP8266WiFi.h>
#else
# include <WiFi.h>
#endif

#include <uuid/console.h>
#include <uuid/log.h>

namespace fridge {

class Network {
public:
	static void start();
	static void connect();
	static void reconnect();
	static void disconnect();
	static void scan(uuid::console::Shell &shell);
	static void print_status(uuid::console::Shell &shell);

private:
	Network() = delete;

	static void sta_mode_connected(const WiFiEventStationModeConnected &event);
	static void sta_mode_disconnected(const WiFiEventStationModeDisconnected &event);
	static void sta_mode_got_ip(const WiFiEventStationModeGotIP &event);
	static void sta_mode_dhcp_timeout();

	static uuid::log::Logger logger_;
	static ::WiFiEventHandler sta_mode_connected_;
	static ::WiFiEventHandler sta_mode_disconnected_;
	static ::WiFiEventHandler sta_mode_got_ip_;
	static ::WiFiEventHandler sta_mode_dhcp_timeout_;
};

} // namespace fridge

#endif
