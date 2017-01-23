/*
 * Anope IRC Services
 *
 * Copyright (C) 2012-2016 Anope Team <team@anope.org>
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

#ifndef DNS_H
#define DNS_H

namespace DNS
{
	/** Valid query types
	 */
	enum QueryType
	{
		/* Nothing */
		QUERY_NONE,
		/* A simple A lookup */
		QUERY_A = 1,
		/* An authoritative name server */
		QUERY_NS = 2,
		/* A CNAME lookup */
		QUERY_CNAME = 5,
		/* Start of a zone of authority */
		QUERY_SOA = 6,
		/* Reverse DNS lookup */
		QUERY_PTR = 12,
		/* IPv6 AAAA lookup */
		QUERY_AAAA = 28,
		/* Zone transfer */
		QUERY_AXFR = 252,
		/* A lookup for any record */
		QUERY_ANY = 255
	};

	/** Flags that can be AND'd into DNSPacket::flags to receive certain values
	 */
	enum
	{
		QUERYFLAGS_QR = 0x8000,
		QUERYFLAGS_OPCODE = 0x7800,
		QUERYFLAGS_OPCODE_NOTIFY = 0x2000,
		QUERYFLAGS_AA = 0x400,
		QUERYFLAGS_TC = 0x200,
		QUERYFLAGS_RD = 0x100,
		QUERYFLAGS_RA = 0x80,
		QUERYFLAGS_Z = 0x70,
		QUERYFLAGS_RCODE = 0xF
	};

	enum Error
	{
		ERROR_NONE,
		ERROR_UNKNOWN,
		ERROR_UNLOADED,
		ERROR_TIMEDOUT,
		ERROR_NOT_AN_ANSWER,
		ERROR_NONSTANDARD_QUERY,
		ERROR_FORMAT_ERROR,
		ERROR_SERVER_FAILURE,
		ERROR_DOMAIN_NOT_FOUND,
		ERROR_NOT_IMPLEMENTED,
		ERROR_REFUSED,
		ERROR_NO_RECORDS,
		ERROR_INVALIDTYPE
	};

	struct Question
	{
		Anope::string name;
		QueryType type;
		unsigned short qclass;

		Question() : type(QUERY_NONE), qclass(0) { }
		Question(const Anope::string &n, QueryType t, unsigned short c = 1) : name(n), type(t), qclass(c) { }
		inline bool operator==(const Question & other) const { return name == other.name && type == other.type && qclass == other.qclass; }

		struct hash
		{
			size_t operator()(const Question &q) const
			{
				return Anope::hash_ci()(q.name);
			}
		};
	};

	struct ResourceRecord : Question
	{
		unsigned int ttl;
		Anope::string rdata;
		time_t created;

		ResourceRecord(const Anope::string &n, QueryType t, unsigned short c = 1) : Question(n, t, c), ttl(0), created(Anope::CurTime) { }
		ResourceRecord(const Question &q) : Question(q), ttl(0), created(Anope::CurTime) { }
	};

	struct Query
	{
		std::vector<Question> questions;
		std::vector<ResourceRecord> answers, authorities, additional;
		Error error;

		Query() : error(ERROR_NONE) { }
		Query(const Question &q) : error(ERROR_NONE) { questions.push_back(q); }
	};

	class ReplySocket;
	class Request;

	/** DNS manager
	 */
	class Manager : public Service
	{
	 public:
		static constexpr const char *NAME = "dns/manager";

		Manager(Module *creator) : Service(creator, NAME) { }
		virtual ~Manager() { }

		virtual void Process(Request *req) anope_abstract;
		virtual void RemoveRequest(Request *req) anope_abstract;

		virtual bool HandlePacket(ReplySocket *s, const unsigned char *const data, int len, sockaddrs *from) anope_abstract;

		virtual void UpdateSerial() anope_abstract;
		virtual void Notify(const Anope::string &zone) anope_abstract;
		virtual uint32_t GetSerial() const anope_abstract;
	};

	/** A DNS query.
	 */
	class Request : public Timer, public Question
	{
		Manager *manager;
	 public:
		/* Use result cache if available */
		bool use_cache;
		/* Request id */
	 	unsigned short id;
	 	/* Creator of this request */
		Module *creator;

		Request(Manager *mgr, Module *c, const Anope::string &addr, QueryType qt, bool cache = false) : Timer(0), Question(addr, qt), manager(mgr),
			use_cache(cache), id(0), creator(c) { }

		virtual ~Request()
		{
			manager->RemoveRequest(this);
		}

		/** Called when this request succeeds
		 * @param r The query sent back from the nameserver
		 */
		virtual void OnLookupComplete(const Query *r) anope_abstract;

		/** Called when this request fails or times out.
		 * @param r The query sent back from the nameserver, check the error code.
		 */
		virtual void OnError(const Query *r) { }

		/** Used to time out the query, xalls OnError and lets the TimerManager
		 * delete this request.
		 */
		void Tick(time_t) override
		{
			Anope::Logger.Debug2("Resolver: timeout for query {0}", this->name);
			Query rr(*this);
			rr.error = ERROR_TIMEDOUT;
			this->OnError(&rr);
		}
	};

} // namespace DNS

namespace Event
{
	struct CoreExport DnsRequest : Events
	{
		static constexpr const char *NAME = "dnsrequest";

		using Events::Events;

		/** Called when a DNS request (question) is recieved.
		 * @param req The dns request
		 * @param reply The reply that will be sent
		 */
		virtual void OnDnsRequest(DNS::Query &req, DNS::Query *reply) anope_abstract;
	};
}

#endif // DNS_H


