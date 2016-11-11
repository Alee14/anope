/*
 * Anope IRC Services
 *
 * Copyright (C) 2003-2016 Anope Team <team@anope.org>
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

#include "services.h"
#include "timers.h"
#include "config.h"
#include "bots.h"
#include "socketengine.h"
#include "uplink.h"
#include "event.h"
#include "modules.h"

#ifndef _WIN32
#include <limits.h>
#else
#include <process.h>
#endif

/* Command-line options: */
int Anope::Debug = 0;
bool Anope::ReadOnly = false, Anope::NoFork = false, Anope::NoThird = false, Anope::NoExpire = false, Anope::ProtocolDebug = false;
Anope::string Anope::ServicesDir;
Anope::string Anope::ServicesBin;

int Anope::ReturnValue = 0;
sig_atomic_t Anope::Signal = 0;
bool Anope::Quitting = false;
bool Anope::Restarting = false;
Anope::string Anope::QuitReason;

static Anope::string BinaryDir;       /* Full path to services bin directory */

time_t Anope::StartTime = time(NULL);
time_t Anope::CurTime = time(NULL);

int Anope::CurrentUplink = -1;

class ExpireTimer : public Timer
{
 public:
	ExpireTimer(time_t timeout) : Timer(timeout, Anope::CurTime, true) { }

	void Tick(time_t) override
	{
		EventManager::Get()->Dispatch(&Event::ExpireTick::OnExpireTick);
	}
};

/** The following comes from InspIRCd to get the full path of the Anope executable
 */
static Anope::string GetFullProgDir(const Anope::string &argv0)
{
	char buffer[PATH_MAX];
#ifdef _WIN32
	/* Windows has specific API calls to get the EXE path that never fail.
	 * For once, Windows has something of use, compared to the POSIX code
	 * for this, this is positively neato.
	 */
	if (GetModuleFileName(NULL, buffer, PATH_MAX))
	{
		Anope::string fullpath = buffer;
		Anope::string::size_type n = fullpath.rfind("\\");
		Anope::ServicesBin = fullpath.substr(n + 1, fullpath.length());
		return fullpath.substr(0, n);
	}
#else
	// Get the current working directory
	if (getcwd(buffer, PATH_MAX))
	{
		Anope::string remainder = argv0;

		Anope::ServicesBin = remainder;
		Anope::string::size_type n = Anope::ServicesBin.rfind("/");
		Anope::string fullpath;
		if (Anope::ServicesBin[0] == '/')
			fullpath = Anope::ServicesBin.substr(0, n);
		else
			fullpath = Anope::string(buffer) + "/" + Anope::ServicesBin.substr(0, n);
		Anope::ServicesBin = Anope::ServicesBin.substr(n + 1, remainder.length());
		return fullpath;
	}
#endif
	return "/";
}

/* Main routine.  (What does it look like? :-) ) */

int main(int ac, char **av, char **envp)
{
	BinaryDir = GetFullProgDir(av[0]);
	if (BinaryDir[BinaryDir.length() - 1] == '.')
		BinaryDir = BinaryDir.substr(0, BinaryDir.length() - 2);

#ifdef _WIN32
	Anope::string::size_type n = BinaryDir.rfind('\\');
#else
	Anope::string::size_type n = BinaryDir.rfind('/');
#endif
	Anope::ServicesDir = BinaryDir.substr(0, n);

#ifdef _WIN32
	/* Clean out the module runtime directory prior to running, just in case files were left behind during a previous run */
	ModuleManager::CleanupRuntimeDirectory();

	OnStartup();
#endif

	try
	{
		/* General initialization first */
		Anope::Init(ac, av);
	}
	catch (const CoreException &ex)
	{
		Log() << ex.GetReason();
		return -1;
	}

	try
	{
		Uplink::Connect();
	}
	catch (const SocketException &ex)
	{
		Log(LOG_TERMINAL) << "Unable to connect to uplink #" << (Anope::CurrentUplink + 1) << " (" << Config->Uplinks[Anope::CurrentUplink].host << ":" << Config->Uplinks[Anope::CurrentUplink].port << "): " << ex.GetReason();
	}

	/* Set up timers */
	time_t last_check = Anope::CurTime;
	ExpireTimer expireTimer(Config->GetBlock("options")->Get<time_t>("expiretimeout", "30m"));

	/*** Main loop. ***/
	while (!Anope::Quitting)
	{
		Log(LOG_DEBUG_2) << "Top of main loop";

		/* Process timers */
		if (Anope::CurTime - last_check >= Config->TimeoutCheck)
		{
			TimerManager::TickTimers(Anope::CurTime);
			last_check = Anope::CurTime;
		}

		/* Process the socket engine */
		SocketEngine::Process();

		if (Anope::Signal)
			Anope::HandleSignal();
	}

	if (Anope::Restarting)
	{
		EventManager::Get()->Dispatch(&Event::Restart::OnRestart);
	}
	else
	{
		EventManager::Get()->Dispatch(&Event::Shutdown::OnShutdown);
	}

	if (Anope::QuitReason.empty())
		Anope::QuitReason = "Terminating, reason unknown";
	Log() << Anope::QuitReason;

	delete UplinkSock;

	ModuleManager::UnloadAll();
	SocketEngine::Shutdown();
	for (Module *m; (m = ModuleManager::FindFirstOf(PROTOCOL)) != NULL;)
		ModuleManager::UnloadModule(m, NULL);

#ifdef _WIN32
	ModuleManager::CleanupRuntimeDirectory();

	OnShutdown();
#endif

	if (Anope::Restarting)
	{
		chdir(BinaryDir.c_str());
		Anope::string sbin = "./" + Anope::ServicesBin;
		av[0] = const_cast<char *>(sbin.c_str());
		execve(Anope::ServicesBin.c_str(), av, envp);
		Log() << "Restart failed";
		Anope::ReturnValue = -1;
	}

	return Anope::ReturnValue;
}
