/*
 * (C) 2014 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 */

#pragma once

#include "event.h"
#include "channels.h"
#include "modules/nickserv.h"
#include "modules/memoserv.h"
#include "bots.h"

namespace ChanServ
{
	static struct
	{
		Anope::string name;
		Anope::string desc;
	} descriptions[] = {
		{"ACCESS_CHANGE", _("Allowed to modify the access list")},
		{"ACCESS_LIST", _("Allowed to view the access list")},
		{"AKICK", _("Allowed to use the AKICK command")},
		{"ASSIGN", _("Allowed to assign/unassign a bot")},
		{"AUTOHALFOP", _("Automatic halfop upon join")},
		{"AUTOOP", _("Automatic channel operator status upon join")},
		{"AUTOOWNER", _("Automatic owner upon join")},
		{"AUTOPROTECT", _("Automatic protect upon join")},
		{"AUTOVOICE", _("Automatic voice on join")},
		{"BADWORDS", _("Allowed to modify channel badwords list")},
		{"BAN", _("Allowed to ban users")},
		{"FANTASIA", _("Allowed to use fantasy commands")},
		{"FOUNDER", _("Allowed to issue commands restricted to channel founders")},
		{"GETKEY", _("Allowed to use GETKEY command")},
		{"GREET", _("Greet message displayed on join")},
		{"HALFOP", _("Allowed to (de)halfop users")},
		{"HALFOPME", _("Allowed to (de)halfop him/herself")},
		{"INFO", _("Allowed to get full INFO output")},
		{"INVITE", _("Allowed to use the INVITE command")},
		{"KICK", _("Allowed to use the KICK command")},
		{"MEMO", _("Allowed to read channel memos")},
		{"MODE", _("Allowed to use the MODE command")},
		{"NOKICK", _("Prevents users being kicked by Services")},
		{"OP", _("Allowed to (de)op users")},
		{"OPME", _("Allowed to (de)op him/herself")},
		{"OWNER", _("Allowed to (de)owner users")},
		{"OWNERME", _("Allowed to (de)owner him/herself")},
		{"PROTECT", _("Allowed to (de)protect users")},
		{"PROTECTME", _("Allowed to (de)protect him/herself")},
		{"SAY", _("Allowed to use SAY and ACT commands")},
		{"SET", _("Allowed to set channel settings")},
		{"SIGNKICK", _("No signed kick when SIGNKICK LEVEL is used")},
		{"TOPIC", _("Allowed to change channel topics")},
		{"UNBAN", _("Allowed to unban users")},
		{"VOICE", _("Allowed to (de)voice users")},
		{"VOICEME", _("Allowed to (de)voice him/herself")}
	};

	/* A privilege, probably configured using a privilege{} block. Most
	 * commands require specific privileges to be executed.
	 */
	struct CoreExport Privilege
	{
		Anope::string name;
		Anope::string desc;
		/* Rank relative to other privileges */
		int rank;
		int level;

		Privilege(const Anope::string &n, const Anope::string &d, int r, int l) : name(n), desc(d), rank(r), level(l)
		{
			if (this->desc.empty())
				for (unsigned j = 0; j < sizeof(descriptions) / sizeof(*descriptions); ++j)
					if (descriptions[j].name.equals_ci(name))
						this->desc = descriptions[j].desc;
		}

		bool operator==(const Privilege &other) const
		{
			return this->name.equals_ci(other.name);
		}
	};

	class Channel;
	using registered_channel_map = Anope::hash_map<Channel *>;

	class Level : public Serialize::Object
	{
	 public:
		static constexpr const char *const NAME = "level";
		
		using Serialize::Object::Object;

		virtual Channel *GetChannel() anope_abstract;
		virtual void SetChannel(Channel *) anope_abstract;

		virtual Anope::string GetName() anope_abstract;
		virtual void SetName(const Anope::string &) anope_abstract;

		virtual int GetLevel() anope_abstract;
		virtual void SetLevel(const int &) anope_abstract;
	};

	class Mode : public Serialize::Object
	{
	 public:
		static constexpr const char *const NAME = "mlockmode";
		 
		using Serialize::Object::Object;

		virtual Channel *GetChannel() anope_abstract;
		virtual void SetChannel(Channel *) anope_abstract;

		virtual Anope::string GetMode() anope_abstract;
		virtual void SetMode(const Anope::string &) anope_abstract;

		virtual Anope::string GetParam() anope_abstract;
		virtual void SetParam(const Anope::string &) anope_abstract;
	};

	class ChanServService : public Service
	{
	 public:
		static constexpr const char *NAME = "chanserv";
		
		ChanServService(Module *m) : Service(m, NAME)
		{
		}

		virtual Channel *Find(const Anope::string &name) anope_abstract;
		virtual registered_channel_map& GetChannels() anope_abstract;

		/* Have ChanServ hold the channel, that is, join and set +nsti and wait
		 * for a few minutes so no one can join or rejoin.
		 */
		virtual void Hold(::Channel *c) anope_abstract;

		virtual void AddPrivilege(Privilege p) anope_abstract;
		virtual void RemovePrivilege(Privilege &p) anope_abstract;
		virtual Privilege *FindPrivilege(const Anope::string &name) anope_abstract;
		virtual std::vector<Privilege> &GetPrivileges() anope_abstract;
		virtual void ClearPrivileges() anope_abstract;
	};
	
	extern ChanServService *service;

	inline Channel *Find(const Anope::string name)
	{
		return service ? service->Find(name) : nullptr;
	}

	namespace Event
	{
		struct CoreExport PreChanExpire : Events
		{
			static constexpr const char *NAME = "prechanexpire";

			using Events::Events;
			
			/** Called before a channel expires
			 * @param ci The channel
			 * @param expire Set to true to allow the chan to expire
			 */
			virtual void OnPreChanExpire(Channel *ci, bool &expire) anope_abstract;
		};

		struct CoreExport ChanExpire : Events
		{
			static constexpr const char *NAME = "chanexpire";

			using Events::Events;
			
			/** Called before a channel expires
			 * @param ci The channel
			 */
			virtual void OnChanExpire(Channel *ci) anope_abstract;
		};
	}

	/* It matters that Base is here before Extensible (it is inherited by Serializable)
	 */
	class CoreExport Channel : public Serialize::Object
	{
	 public:
		::Channel *c = nullptr;                               /* Pointer to channel, if the channel exists */

	 protected:
		using Serialize::Object::Object;

	 public:
		static constexpr const char *const NAME = "channel";
		
		virtual Anope::string GetName() anope_abstract;
		virtual void SetName(const Anope::string &) anope_abstract;

		virtual Anope::string GetDesc() anope_abstract;
		virtual void SetDesc(const Anope::string &) anope_abstract;

		virtual time_t GetTimeRegistered() anope_abstract;
		virtual void SetTimeRegistered(const time_t &) anope_abstract;

		virtual time_t GetLastUsed() anope_abstract;
		virtual void SetLastUsed(const time_t &) anope_abstract;

		virtual Anope::string GetLastTopic() anope_abstract;
		virtual void SetLastTopic(const Anope::string &) anope_abstract;

		virtual Anope::string GetLastTopicSetter() anope_abstract;
		virtual void SetLastTopicSetter(const Anope::string &) anope_abstract;

		virtual time_t GetLastTopicTime() anope_abstract;
		virtual void SetLastTopicTime(const time_t &) anope_abstract;

		virtual int16_t GetBanType() anope_abstract;
		virtual void SetBanType(const int16_t &) anope_abstract;

		virtual time_t GetBanExpire() anope_abstract;
		virtual void SetBanExpire(const time_t &) anope_abstract;

		virtual BotInfo *GetBI() anope_abstract;
		virtual void SetBI(BotInfo *) anope_abstract;

		virtual ServiceBot *GetBot() anope_abstract;
		virtual void SetBot(ServiceBot *) anope_abstract;

		/** Is the user the real founder?
		 * @param user The user
		 * @return true or false
		 */
		virtual bool IsFounder(const User *user) anope_abstract;

		/** Change the founder of the channel
		 * @params nc The new founder
		 */
		virtual void SetFounder(NickServ::Account *nc) anope_abstract;

		/** Get the founder of the channel
		 * @return The founder
		 */
		virtual NickServ::Account *GetFounder() anope_abstract;

		virtual void SetSuccessor(NickServ::Account *nc) anope_abstract;
		virtual NickServ::Account *GetSuccessor() anope_abstract;

		/** Find which bot should send mode/topic/etc changes for this channel
		 * @return The bot
		 */
		ServiceBot *WhoSends()
		{
			if (this)
				if (ServiceBot *bi = GetBot())
					return bi;

			ServiceBot *ChanServ = Config->GetClient("ChanServ");
			if (ChanServ)
				return ChanServ;

#warning "if(this)"
			//XXX
//			if (!BotListByNick->empty())
//				return BotListByNick->begin()->second;

			return NULL;
		}

		/** Get an entry from the channel access list by index
		 *
		 * @param index The index in the access list vector
		 * @return A ChanAccess struct corresponding to the index given, or NULL if outside the bounds
		 *
		 * Retrieves an entry from the access list that matches the given index.
		 */
		virtual ChanAccess *GetAccess(unsigned index) /*const*/ anope_abstract;

		/** Retrieve the access for a user or group in the form of a vector of access entries
		 * (as multiple entries can affect a single user).
		 */
		virtual AccessGroup AccessFor(const User *u, bool updateLastUsed = true) anope_abstract;
		virtual AccessGroup AccessFor(NickServ::Account *nc, bool updateLastUsed = true) anope_abstract;

		/** Get the size of the accss vector for this channel
		 * @return The access vector size
		 */
		virtual unsigned GetAccessCount() /*const*/ anope_abstract;

		/** Clear the entire channel access list
		 *
		 * Clears the entire access list by deleting every item and then clearing the vector.
		 */
		virtual void ClearAccess() anope_abstract;

		/** Add an akick entry to the channel by NickServ::Account
		 * @param user The user who added the akick
		 * @param akicknc The nickcore being akicked
		 * @param reason The reason for the akick
		 * @param t The time the akick was added, defaults to now
		 * @param lu The time the akick was last used, defaults to never
		 */
		virtual AutoKick* AddAkick(const Anope::string &user, NickServ::Account *akicknc, const Anope::string &reason, time_t t = Anope::CurTime, time_t lu = 0) anope_abstract;

		/** Add an akick entry to the channel by reason
		 * @param user The user who added the akick
		 * @param mask The mask of the akick
		 * @param reason The reason for the akick
		 * @param t The time the akick was added, defaults to now
		 * @param lu The time the akick was last used, defaults to never
		 */
		virtual AutoKick* AddAkick(const Anope::string &user, const Anope::string &mask, const Anope::string &reason, time_t t = Anope::CurTime, time_t lu = 0) anope_abstract;

		/** Get an entry from the channel akick list
		 * @param index The index in the akick vector
		 * @return The akick structure, or NULL if not found
		 */
		virtual AutoKick* GetAkick(unsigned index) anope_abstract;

		/** Get the size of the akick vector for this channel
		 * @return The akick vector size
		 */
		virtual unsigned GetAkickCount() anope_abstract;

		/** Clear the whole akick list
		 */
		virtual void ClearAkick() anope_abstract;

		/** Get the level for a privilege
		 * @param priv The privilege name
		 * @return the level
		 * @throws CoreException if priv is not a valid privilege
		 */
		virtual int16_t GetLevel(const Anope::string &priv) anope_abstract;

		/** Set the level for a privilege
		 * @param priv The privilege priv
		 * @param level The new level
		 */
		virtual void SetLevel(const Anope::string &priv, int16_t level) anope_abstract;

		/** Remove a privilege from the channel
		 * @param priv The privilege
		 */
		virtual void RemoveLevel(const Anope::string &priv) anope_abstract;

		/** Clear all privileges from the channel
		 */
		virtual void ClearLevels() anope_abstract;

		/** Gets a ban mask for the given user based on the bantype
		 * of the channel.
		 * @param u The user
		 * @return A ban mask that affects the user
		 */
		virtual Anope::string GetIdealBan(User *u) anope_abstract;

		virtual MemoServ::MemoInfo *GetMemos() anope_abstract;
	};

	enum
	{
		ACCESS_INVALID = -10000,
		ACCESS_FOUNDER = 10001
	};

	/* Represents one entry of an access list on a channel. */
	class CoreExport ChanAccess : public Serialize::Object
	{
	 public:
		static constexpr const char *const NAME = "access";

		Channel *channel = nullptr;
		Serialize::Object *object = nullptr;
		Anope::string creator, mask;
		time_t last_seen = 0, created = 0;

		using Serialize::Object::Object;

		virtual Channel *GetChannel() anope_abstract;
		virtual void SetChannel(Channel *ci) anope_abstract;

		virtual Anope::string GetCreator() anope_abstract;
		virtual void SetCreator(const Anope::string &c) anope_abstract;

		virtual time_t GetLastSeen() anope_abstract;
		virtual void SetLastSeen(const time_t &t) anope_abstract;

		virtual time_t GetCreated() anope_abstract;
		virtual void SetCreated(const time_t &t) anope_abstract;

		virtual Anope::string GetMask() anope_abstract;
		virtual void SetMask(const Anope::string &) anope_abstract;

		virtual Serialize::Object *GetObj() anope_abstract;
		virtual void SetObj(Serialize::Object *) anope_abstract;

		virtual Anope::string Mask() anope_abstract;
		virtual NickServ::Account *GetAccount() anope_abstract;

		/** Check if this access entry matches the given user or account
		 * @param u The user
		 * @param acc The account
		 */
		virtual bool Matches(const User *u, NickServ::Account *acc) anope_abstract;

		/** Check if this access entry has the given privilege.
		 * @param name The privilege name
		 */
		virtual bool HasPriv(const Anope::string &name) anope_abstract;

		/** Serialize the access given by this access entry into a human
		 * readable form. chanserv/access will return a number, chanserv/xop
		 * will be AOP, SOP, etc.
		 */
		virtual Anope::string AccessSerialize() anope_abstract;

		/** Unserialize this access entry from the given data. This data
		 * will be fetched from AccessSerialize.
		 */
		virtual void AccessUnserialize(const Anope::string &data) anope_abstract;

		/* Comparison operators to other Access entries */
		virtual bool operator>(ChanAccess &other)
		{
			const std::vector<Privilege> &privs = service->GetPrivileges();
			for (unsigned i = privs.size(); i > 0; --i)
			{
				bool this_p = this->HasPriv(privs[i - 1].name),
					other_p = other.HasPriv(privs[i - 1].name);

				if (!this_p && !other_p)
					continue;

				return this_p && !other_p;
			}

			return false;
		}

		virtual bool operator<(ChanAccess &other)
		{
			const std::vector<Privilege> &privs = service->GetPrivileges();
			for (unsigned i = privs.size(); i > 0; --i)
			{
				bool this_p = this->HasPriv(privs[i - 1].name),
					other_p = other.HasPriv(privs[i - 1].name);

				if (!this_p && !other_p)
					continue;

				return !this_p && other_p;
			}

			return false;
		}

		bool operator>=(ChanAccess &other)
		{
			return !(*this < other);
		}

		bool operator<=(ChanAccess &other)
		{
			return !(*this > other);
		}
	};
}

