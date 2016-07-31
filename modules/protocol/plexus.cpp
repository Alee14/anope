/*
 * Anope IRC Services
 *
 * Copyright (C) 2005-2016 Anope Team <team@anope.org>
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

#include "module.h"

static Anope::string UplinkSID;

class PlexusProto : public IRCDProto
{
	ServiceReference<IRCDProto> hybrid; // XXX use moddeps + inheritance here
	
 public:
	PlexusProto(Module *creator) : IRCDProto(creator, "hybrid-7.2.3+plexus-3.0.1")
		, hybrid("hybrid")
	{
		DefaultPseudoclientModes = "+oiU";
		CanSVSNick = true;
		CanSVSJoin = true;
		CanSetVHost = true;
		CanSetVIdent = true;
		CanSNLine = true;
		CanSQLine = true;
		CanSQLineChannel = true;
		CanSVSHold = true;
		CanCertFP = true;
		RequiresID = true;
		MaxModes = 4;
	}

	void SendSVSKillInternal(const MessageSource &source, User *targ, const Anope::string &reason) override { hybrid->SendSVSKillInternal(source, targ, reason); }
	void SendGlobalNotice(ServiceBot *bi, const Server *dest, const Anope::string &msg) override { hybrid->SendGlobalNotice(bi, dest, msg); }
	void SendGlobalPrivmsg(ServiceBot *bi, const Server *dest, const Anope::string &msg) override { hybrid->SendGlobalPrivmsg(bi, dest, msg); }
	void SendSQLine(User *u, XLine *x) override { hybrid->SendSQLine(u, x); }
	void SendSQLineDel(XLine *x) override { hybrid->SendSQLineDel(x); }
	void SendSGLineDel(XLine *x) override { hybrid->SendSGLineDel(x); }
	void SendSGLine(User *u, XLine *x) override { hybrid->SendSGLine(u, x); }
	void SendAkillDel(XLine *x) override { hybrid->SendAkillDel(x); }
	void SendAkill(User *u, XLine *x) override { hybrid->SendAkill(u, x); }
	void SendServer(const Server *server) override { hybrid->SendServer(server); }
	void SendChannel(Channel *c) override { hybrid->SendChannel(c); }
	void SendSVSHold(const Anope::string &nick, time_t t) override { hybrid->SendSVSHold(nick, t); }
	void SendSVSHoldDel(const Anope::string &nick) override { hybrid->SendSVSHoldDel(nick); }

	void SendGlobopsInternal(const MessageSource &source, const Anope::string &buf) override
	{
		UplinkSocket::Message(source) << "OPERWALL :" << buf;
	}

	void SendJoin(User *user, Channel *c, const ChannelStatus *status) override
	{
		UplinkSocket::Message(Me) << "SJOIN " << c->creation_time << " " << c->name << " +" << c->GetModes(true, true) << " :" << user->GetUID();
		if (status)
		{
			/* First save the channel status incase uc->Status == status */
			ChannelStatus cs = *status;
			/* If the user is internally on the channel with flags, kill them so that
			 * the stacker will allow this.
			 */
			ChanUserContainer *uc = c->FindUser(user);
			if (uc != NULL)
				uc->status.Clear();

			ServiceBot *setter = ServiceBot::Find(user->GetUID());
			for (size_t i = 0; i < cs.Modes().length(); ++i)
				c->SetMode(setter, ModeManager::FindChannelModeByChar(cs.Modes()[i]), user->GetUID(), false);

			if (uc != NULL)
				uc->status = cs;
		}
	}

	void SendForceNickChange(User *u, const Anope::string &newnick, time_t when) override
	{
		UplinkSocket::Message(Me) << "ENCAP " << u->server->GetName() << " SVSNICK " << u->GetUID() << " " << u->timestamp << " " << newnick << " " << when;
	}

	void SendVhost(User *u, const Anope::string &ident, const Anope::string &host) override
	{
		if (!ident.empty())
			UplinkSocket::Message(Me) << "ENCAP * CHGIDENT " << u->GetUID() << " " << ident;
		UplinkSocket::Message(Me) << "ENCAP * CHGHOST " << u->GetUID() << " " << host;
		u->SetMode(Config->GetClient("HostServ"), "CLOAK");
	}

	void SendVhostDel(User *u) override
	{
		u->RemoveMode(Config->GetClient("HostServ"), "CLOAK");
	}

	void SendConnect() override
	{
		UplinkSocket::Message() << "PASS " << Config->Uplinks[Anope::CurrentUplink].password << " TS 6 :" << Me->GetSID();
		/* CAPAB
		 * QS     - Can handle quit storm removal
		 * EX     - Can do channel +e exemptions
		 * CHW    - Can do channel wall @#
		 * LL     - Can do lazy links
		 * IE     - Can do invite exceptions
		 * EOB    - Can do EOB message
		 * KLN    - Can do KLINE message
		 * GLN    - Can do GLINE message
		 * HUB    - This server is a HUB
		 * AOPS   - Can do anon ops (+a)
		 * UID    - Can do UIDs
		 * ZIP    - Can do ZIPlinks
		 * ENC    - Can do ENCrypted links
		 * KNOCK  - Supports KNOCK
		 * TBURST - Supports TBURST
		 * PARA   - Supports invite broadcasting for +p
		 * ENCAP  - Supports encapsulization of protocol messages
		 * SVS    - Supports services protocol extensions
		 */
		UplinkSocket::Message() << "CAPAB :QS EX CHW IE EOB KLN UNKLN GLN HUB KNOCK TBURST PARA ENCAP SVS";
		/* Make myself known to myself in the serverlist */
		SendServer(Me);
		/*
		 * SVINFO
		 *	  parv[0] = sender prefix
		 *	  parv[1] = TS_CURRENT for the server
		 *	  parv[2] = TS_MIN for the server
		 *	  parv[3] = server is standalone or connected to non-TS only
		 *	  parv[4] = server's idea of UTC time
		 */
		UplinkSocket::Message() << "SVINFO 6 5 0 :" << Anope::CurTime;
	}

	void SendClientIntroduction(User *u) override
	{
		Anope::string modes = "+" + u->GetModes();
		UplinkSocket::Message(Me) << "UID " << u->nick << " 1 " << u->timestamp << " " << modes << " " << u->GetIdent() << " " << u->host << " 255.255.255.255 " << u->GetUID() << " 0 " << u->host << " :" << u->realname;
	}

	void SendModeInternal(const MessageSource &source, User *u, const Anope::string &buf) override
	{
		UplinkSocket::Message(source) << "ENCAP * SVSMODE " << u->GetUID() << " " << u->timestamp << " " << buf;
	}

	void SendLogin(User *u, NickServ::Nick *na) override
	{
		UplinkSocket::Message(Me) << "ENCAP * SU " << u->GetUID() << " " << na->GetAccount()->GetDisplay();
	}

	void SendLogout(User *u) override
	{
		UplinkSocket::Message(Me) << "ENCAP * SU " << u->GetUID();
	}

	void SendTopic(const MessageSource &source, Channel *c) override
	{
		UplinkSocket::Message(source) << "ENCAP * TOPIC " << c->name << " " << c->topic_setter << " " << c->topic_ts << " :" << c->topic;
	}

	void SendSVSJoin(const MessageSource &source, User *user, const Anope::string &chan, const Anope::string &param) override
	{
		UplinkSocket::Message(source) << "ENCAP " << user->server->GetName() << " SVSJOIN " << user->GetUID() << " " << chan;
	}

	void SendSVSPart(const MessageSource &source, User *user, const Anope::string &chan, const Anope::string &param) override
	{
		UplinkSocket::Message(source) << "ENCAP " << user->server->GetName() << " SVSPART " << user->GetUID() << " " << chan;
	}
};

struct IRCDMessageEncap : IRCDMessage
{
	IRCDMessageEncap(Module *creator) : IRCDMessage(creator, "ENCAP", 4) { SetFlag(IRCDMESSAGE_REQUIRE_SERVER); }

	void Run(MessageSource &source, const std::vector<Anope::string> &params) override
	{
		/*
		 * Received: :dev.anope.de ENCAP * SU DukePyrolator DukePyrolator
		 * params[0] = *
		 * params[1] = SU
		 * params[2] = nickname
		 * params[3] = account
		 */
		if (params[1].equals_cs("SU"))
		{
			User *u = User::Find(params[2]);
			NickServ::Account *nc = NickServ::FindAccount(params[3]);
			if (u && nc)
			{
				u->Login(nc);
			}
		}

		/*
		 * Received: :dev.anope.de ENCAP * CERTFP DukePyrolator :3F122A9CC7811DBAD3566BF2CEC3009007C0868F
		 * params[0] = *
		 * params[1] = CERTFP
		 * params[2] = nickname
		 * params[3] = fingerprint
		 */
		else if (params[1].equals_cs("CERTFP"))
		{
			User *u = User::Find(params[2]);
			if (u)
			{
				u->fingerprint = params[3];
				EventManager::Get()->Dispatch(&Event::Fingerprint::OnFingerprint, u);
			}
		}
		return;
	}
};

struct IRCDMessagePass : IRCDMessage
{
	IRCDMessagePass(Module *creator) : IRCDMessage(creator, "PASS", 4) { SetFlag(IRCDMESSAGE_REQUIRE_SERVER); }

	void Run(MessageSource &source, const std::vector<Anope::string> &params) override
	{
		UplinkSID = params[3];
	}
};

struct IRCDMessageServer : IRCDMessage
{
	IRCDMessageServer(Module *creator) : IRCDMessage(creator, "SERVER", 3) { SetFlag(IRCDMESSAGE_REQUIRE_SERVER); }

	/*        0          1  2                       */
	/* SERVER hades.arpa 1 :ircd-hybrid test server */
	void Run(MessageSource &source, const std::vector<Anope::string> &params) override
	{
		/* Servers other than our immediate uplink are introduced via SID */
		if (params[1] != "1")
			return;

		new Server(source.GetServer() == NULL ? Me : source.GetServer(), params[0], 1, params[2], UplinkSID);
	}
};

struct IRCDMessageUID : IRCDMessage
{
	IRCDMessageUID(Module *creator) : IRCDMessage(creator, "UID", 11) { SetFlag(IRCDMESSAGE_REQUIRE_SERVER); }

	/*
	   params[0] = nick
	   params[1] = hop
	   params[2] = ts
	   params[3] = modes
	   params[4] = user
	   params[5] = host
	   params[6] = IP
	   params[7] = UID
	   params[8] = services stamp
	   params[9] = realhost
	   params[10] = info
	*/
	// :42X UID Adam 1 1348535644 +aow Adam 192.168.0.5 192.168.0.5 42XAAAAAB 0 192.168.0.5 :Adam
	void Run(MessageSource &source, const std::vector<Anope::string> &params) override
	{
		/* An IP of 0 means the user is spoofed */
		Anope::string ip = params[6];
		if (ip == "0")
			ip.clear();

		time_t ts;
		try
		{
			ts = convertTo<time_t>(params[2]);
		}
		catch (const ConvertException &)
		{
			ts = Anope::CurTime;
		}

		NickServ::Nick *na = NULL;
		try
		{
			if (params[8].is_pos_number_only() && convertTo<time_t>(params[8]) == ts)
				na = NickServ::FindNick(params[0]);
		}
		catch (const ConvertException &) { }
		if (params[8] != "0" && !na)
			na = NickServ::FindNick(params[8]);

		User::OnIntroduce(params[0], params[4], params[9], params[5], ip, source.GetServer(), params[10], ts, params[3], params[7], na ? na->GetAccount() : NULL);
	}
};

class ProtoPlexus : public Module
{
	Module *m_hybrid;

	PlexusProto ircd_proto;

	/* Core message handlers */
	Message::Away message_away;
	Message::Capab message_capab;
	Message::Error message_error;
	Message::Invite message_invite;
	Message::Kick message_kick;
	Message::Kill message_kill;
	Message::Mode message_mode;
	Message::MOTD message_motd;
	Message::Notice message_notice;
	Message::Part message_part;
	Message::Ping message_ping;
	Message::Privmsg message_privmsg;
	Message::Quit message_quit;
	Message::SQuit message_squit;
	Message::Stats message_stats;
	Message::Time message_time;
	Message::Topic message_topic;
	Message::Version message_version;
	Message::Whois message_whois;

	/* Hybrid message handlers */
	ServiceAlias message_bmask, message_eob, message_join, message_nick, message_sid, message_sjoin,
			message_tburst, message_tmode;

	/* Our message handlers */
	IRCDMessageEncap message_encap;
	IRCDMessagePass message_pass;
	IRCDMessageServer message_server;
	IRCDMessageUID message_uid;

 public:
	ProtoPlexus(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, PROTOCOL | VENDOR)
		, ircd_proto(this)
		, message_away(this)
		, message_capab(this)
		, message_error(this)
		, message_invite(this)
		, message_kick(this)
		, message_kill(this)
		, message_mode(this)
		, message_motd(this)
		, message_notice(this)
		, message_part(this)
		, message_ping(this)
		, message_privmsg(this)
		, message_quit(this)
		, message_squit(this)
		, message_stats(this)
		, message_time(this)
		, message_topic(this)
		, message_version(this)
		, message_whois(this)

		, message_bmask("IRCDMessage", "plexus/bmask", "hybrid/bmask")
		, message_eob("IRCDMessage", "plexus/eob", "hybrid/eob")
		, message_join("IRCDMessage", "plexus/join", "hybrid/join")
		, message_nick("IRCDMessage", "plexus/nick", "hybrid/nick")
		, message_sid("IRCDMessage", "plexus/sid", "hybrid/sid")
		, message_sjoin("IRCDMessage", "plexus/sjoin", "hybrid/sjoin")
		, message_tburst("IRCDMessage", "plexus/tburst", "hybrid/tburst")
		, message_tmode("IRCDMessage", "plexus/tmode", "hybrid/tmode")

		, message_encap(this)
		, message_pass(this)
		, message_server(this)
		, message_uid(this)
	{

		if (ModuleManager::LoadModule("hybrid", User::Find(creator)) != MOD_ERR_OK)
			throw ModuleException("Unable to load hybrid");
		m_hybrid = ModuleManager::FindModule("hybrid");
		if (!m_hybrid)
			throw ModuleException("Unable to find hybrid");
//		if (!hybrid)
//			throw ModuleException("No protocol interface for hybrid");
	}

	~ProtoPlexus()
	{
		m_hybrid = ModuleManager::FindModule("hybrid");
		ModuleManager::UnloadModule(m_hybrid, NULL);
	}
};

MODULE_INIT(ProtoPlexus)
