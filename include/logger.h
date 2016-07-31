/*
 * Anope IRC Services
 *
 * Copyright (C) 2010-2016 Anope Team <team@anope.org>
 *
 * This file is part of Anope. Anope is free software; you can
 * redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "anope.h"
#include "defs.h"

enum LogType
{
	/* Used whenever an administrator uses an administrative comand */
	LOG_ADMIN,
	/* Used whenever an administrator overides something, such as adding
	 * access to a channel where they don't have permission to.
	 */
	LOG_OVERRIDE,
	/* Any other command usage */
	LOG_COMMAND,
	LOG_SERVER,
	LOG_CHANNEL,
	LOG_USER,
	LOG_MODULE,
	LOG_NORMAL,
	LOG_TERMINAL,
	LOG_RAWIO,
	LOG_DEBUG,
	LOG_DEBUG_2,
	LOG_DEBUG_3,
	LOG_DEBUG_4
};

struct LogFile
{
	Anope::string filename;
	std::ofstream stream;

	LogFile(const Anope::string &name);
	~LogFile();
	const Anope::string &GetName() const;
};

/* Represents a single log message */
class CoreExport Log
{
 public:
 	/* Bot that should log this message */
	ServiceBot *bi;
	/* For commands, the user executing the command, but might not always exist */
	User *u;
	/* For commands, the account executing the command, but will not always exist */
	NickServ::Account *nc;
	/* For commands, the command being executed */
	Command *c;
	/* For commands, the command source */
	CommandSource *source;
	/* Used for LOG_CHANNEL */
	Channel *chan;
	/* For commands, the channel the command was executed on, will not always exist */
	ChanServ::Channel *ci;
	/* For LOG_SERVER */
	Server *s;
	/* For LOG_MODULE */
	Module *m;
	LogType type;
	Anope::string category;

	std::stringstream buf;

	Log(LogType type = LOG_NORMAL, const Anope::string &category = "", ServiceBot *bi = NULL);

	/* LOG_COMMAND/OVERRIDE/ADMIN */
	Log(LogType type, CommandSource &source, Command *c, ChanServ::Channel *ci = NULL);

	/* LOG_CHANNEL */
	Log(User *u, Channel *c, const Anope::string &category = "");

	/* LOG_USER */
	Log(User *u, const Anope::string &category = "", ServiceBot *bi = NULL);

	/* LOG_SERVER */
	Log(Server *s, const Anope::string &category = "", ServiceBot *bi = NULL);

	Log(ServiceBot *b, const Anope::string &category = "");

	Log(Module *m, const Anope::string &category = "", ServiceBot *bi = NULL);

	~Log();

 private:
	Anope::string FormatSource() const;
	Anope::string FormatCommand() const;

 public:
	Anope::string BuildPrefix() const;

	template<typename T> Log &operator<<(T val)
	{
		this->buf << val;
		return *this;
	}
};

/* Configured in the configuration file, actually does the message logging */
class CoreExport LogInfo
{
 public:
 	ServiceBot *bot;
	std::vector<Anope::string> targets;
	std::vector<LogFile *> logfiles;
	int last_day;
	std::vector<Anope::string> sources;
	int log_age;
	std::vector<Anope::string> admin;
	std::vector<Anope::string> override;
	std::vector<Anope::string> commands;
	std::vector<Anope::string> servers;
	std::vector<Anope::string> users;
	std::vector<Anope::string> channels;
	std::vector<Anope::string> normal;
	bool raw_io;
	bool debug;

	LogInfo(int logage, bool rawio, bool debug);

	~LogInfo();

	void OpenLogFiles();

	bool HasType(LogType ltype, const Anope::string &type) const;

	/* Logs the message l if configured to */
	void ProcessMessage(const Log *l);
};

