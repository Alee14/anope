/* NickServ core functions
 *
 * (C) 2003-2009 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 *
 * $Id$
 *
 */
/*************************************************************************/

#include "module.h"

void myNickServHelp(User *u);

class CommandNSRecover : public Command
{
 public:
	CommandNSRecover() : Command("RECOVER", 1, 2)
	{
	}

	CommandResult Execute(User *u, std::vector<std::string> &params)
	{
		char *nick = params[0].c_str();
		char *pass = params.size() > 1 ? params[1].c_str() : NULL;
		NickAlias *na;
		User *u2;

		if (!(u2 = finduser(nick)))
			notice_lang(s_NickServ, u, NICK_X_NOT_IN_USE, nick);
		else if (!(na = u2->na))
			notice_lang(s_NickServ, u, NICK_X_NOT_REGISTERED, nick);
		else if (na->status & NS_FORBIDDEN)
			notice_lang(s_NickServ, u, NICK_X_FORBIDDEN, na->nick);
		else if (na->nc->flags & NI_SUSPENDED)
			notice_lang(s_NickServ, u, NICK_X_SUSPENDED, na->nick);
		else if (!stricmp(nick, u->nick))
			notice_lang(s_NickServ, u, NICK_NO_RECOVER_SELF);
		else if (pass)
		{
			int res = enc_check_password(pass, na->nc->pass);

			if (res == 1)
			{
				char relstr[192];

				notice_lang(s_NickServ, u2, FORCENICKCHANGE_NOW);
				collide(na, 0);

				/* Convert NSReleaseTimeout seconds to string format */
				duration(u2->na, relstr, sizeof(relstr), NSReleaseTimeout);

				notice_lang(s_NickServ, u, NICK_RECOVERED, s_NickServ, nick, relstr);
			}
			else
			{
				notice_lang(s_NickServ, u, ACCESS_DENIED);
				if (!res)
				{
					alog("%s: RECOVER: invalid password for %s by %s!%s@%s", s_NickServ, nick, u->nick, u->GetIdent().c_str(), u->host);
					bad_password(u);
				}
			}
		}
		else
		{
			if (group_identified(u, na->nc) || (!(na->nc->flags & NI_SECURE) && is_on_access(u, na->nc)))
			{
				notice_lang(s_NickServ, u2, FORCENICKCHANGE_NOW);
				collide(na, 0);
				notice_lang(s_NickServ, u, NICK_RECOVERED, s_NickServ, nick);
			}
			else
				notice_lang(s_NickServ, u, ACCESS_DENIED);
		}
		return MOD_CONT;
	}

	bool OnHelp(User *u, const std::string &subcommand)
	{
		char relstr[192];

		/* Convert NSReleaseTimeout seconds to string format */
		duration(u->na, relstr, sizeof(relstr), NSReleaseTimeout);

		notice_help(s_NickServ, u, NICK_HELP_RECOVER, relstr);
		do_help_limited(s_NickServ, u, this);

		return MOD_CONT;
	}

	void OnSyntaxError(User *u)
	{
		syntax_error(s_NickServ, u, "RECOVER", NICK_RECOVER_SYNTAX);
	}
};

class NSRecover : public Module
{
 public:
	NSRecover(const std::string &modname, const std::string &creator) : Module(modname, creator)
	{
		this->SetAuthor("Anope");
		this->SetVersion("$Id$");
		this->SetType(CORE);

		this->AddCommand(NICKSERV, new CommandNSRecover(), MOD_UNIQUE);

		this->SetNickHelp(myNickServHelp);
	}
};

/**
 * Add the help response to anopes /ns help output.
 * @param u The user who is requesting help
 **/
void myNickServHelp(User *u)
{
	notice_lang(s_NickServ, u, NICK_HELP_CMD_RECOVER);
}

MODULE_INIT("ns_recover", NSRecover)
