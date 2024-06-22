/*
* TLS Session Manager
* (C) 2011-2023 Jack Lloyd
*     2022-2023 René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SESSION_MANAGER_H_
#define BOTAN_TLS_SESSION_MANAGER_H_

#include <botan/mutex.h>
#include <botan/tls_session.h>
#include <botan/types.h>

#if defined(BOTAN_HAS_TLS_13)
   #include <botan/tls_psk_identity_13.h>
#endif

#include <chrono>
#include <map>
#include <utility>
#include <variant>

namespace Botan {
class RandomNumberGenerator;
}

namespace Botan::TLS {

class Callbacks;
class Policy;

/**
* Session_Manager is an interface to systems which can save session parameters
* for supporting session resumption.
*
* Saving sessions is done on a best-effort basis; an implementation is
* allowed to drop sessions due to space constraints or other issues.
*
* Implementations should strive to be thread safe. This base class provides a
* recursive mutex (via Session_Manager::mutex()). Derived classes may simply
* reuse this for their own locking.
*/
class BOTAN_PUBLIC_API(3, 0) Session_Manager {
   public:
      Session_Manager(const std::shared_ptr<RandomNumberGenerator>& rng);

      /**
       * @brief Save a new Session and assign a Session_Handle (TLS Server)
       *
       * Save a new session on a best effort basis; the manager may not in fact
       * be able to save the session for whatever reason; this is not an error.
       * Callers cannot assume that calling establish() followed immediately by
       * retrieve() or choose_from_offered_tickets() will result in a successful
       * lookup. In case no session was stored, std::nullopt is returned.
       *
       * This method is only called on TLS servers.
       *
       * Note that implementations will silently refrain from sending session
       * tickets to the client when this method returns std::nullopt.
       *
       * @param session to save
       * @param id to use (instead of an ID chosen by the manager)
       * @param tls12_no_ticket disable tickets for this establishment
       *                        (set when TLS 1.2 client does not support them)
       * @return a Session_Handle containing either an ID or a ticket
       *         if the session was saved, otherwise std::nullopt
       */
      virtual std::optional<Session_Handle> establish(const Session& session,
                                                      const std::optional<Session_ID>& id = std::nullopt,
                                                      bool tls12_no_ticket = false);

      /**
       * @brief Save a Session under a Session_Handle (TLS Client)
       *
       * Save a session on a best effort basis; the manager may not in fact be
       * able to save the session for whatever reason; this is not an error.
       * Callers cannot assume that calling store() followed immediately by
       * find() will result in a successful lookup.
       *
       * In contrast to establish(), this stores sessions that were created by
       * the server along with a Session_Handle also coined by the server.
       *
       * This method is only called on TLS clients.
       *
       * @param session to save
       * @param handle a Session_Handle on which this session shoud by stored
       */
      virtual void store(const Session& session, const Session_Handle& handle) = 0;

#if defined(BOTAN_HAS_TLS_13)
      /**
       * Lets the server application choose a PSK to use for a new TLS
       * connection. Implementers must make sure that the PSK's associated
       * hash function is equal to the passed @p hash_function.
       *
       * RFC 8446 4.2.11
       *    The server MUST ensure that it selects a compatible PSK (if any)
       *    and cipher suite.
       *
       * The default implementation simply tries to retrieve all tickets in
       * the order offered by the peer and picks the first that is found and
       * features a matching hash algorithm.
       *
       * This method is called only by TLS 1.3 servers.
       *
       * @param tickets a list of tickets that were offered by the client
       * @param hash_function the hash algorithm name we are going to use for
       *                      the to-be-negotiated connection
       * @param callbacks callbacks to be used for session policy decisions
       * @param policy policy to be used for session policy decisions
       *
       * @return a std::pair of the Session associated to the choosen PSK and
       *         the index of the selected ticket; std::nullopt if no PSK was
       *         chosen for usage (will result in a full handshake)
       *
       * @note if no PSK is chosen, the server will attempt a regular handshake.
       */
      virtual std::optional<std::pair<Session, uint16_t>> choose_from_offered_tickets(
         const std::vector<PskIdentity>& tickets,
         std::string_view hash_function,
         Callbacks& callbacks,
         const Policy& policy);
#endif

      /**
       * @brief Retrieves a specific session given a @p handle
       *
       * This is typically used by TLS servers to obtain resumption information
       * for a previous call to Session_Manager::establish() when a client
       * requested resumption using the @p handle.
       *
       * Even if the session is found successfully, it is returned only if it
       * passes policy validations. Most notably an expiry check. If the expiry
       * check fails, the default implementation calls Session_Manager::remove()
       * for the provided @p handle.
       *
       * Applications that wish to implement their own Session_Manager may
       * override the default implementation to add further policy checks.
       * Though, typically implementing Session_Manager::retrieve_one() and
       * relying on the default implementation is enough.
       *
       * @param handle     the Session_Handle to be retrieved
       * @param callbacks  callbacks to be used for session policy decisions
       * @param policy     policy to be used for session policy decisions
       * @return           the obtained session or std::nullopt if no session
       *                   was found or the policy checks failed
       */
      virtual std::optional<Session> retrieve(const Session_Handle& handle, Callbacks& callbacks, const Policy& policy);

      /**
       * @brief Find all sessions that match a given server @p info
       *
       * TLS clients use this to obtain session resumption information for a
       * server they are wishing to handshake with. Typically, session info will
       * have been received in prior connections to that same server and stored
       * using Session_Manager::store().
       *
       * The default implementation will invoke Session_Manager::find_some() and
       * filter the result against a policy. Most notably an expiry check.
       * Expired sessions will be removed via Session_Manager::remove().
       *
       * The TLS client implementations will query the session manager exactly
       * once per handshake attempt. If no reuse is desired, the session manager
       * may remove the sessions internally when handing them out to the client.
       * The default implementation adheres to Policy::reuse_session_tickets().
       *
       * For TLS 1.2 the client implementation will attempt a resumption with
       * the first session in the returned list. For TLS 1.3, it will offer all
       * found sessions to the server.
       *
       * Applications that wish to implement their own Session_Manager may
       * override the default implementation to add further policy checks.
       * Though, typically implementing Session_Manager::find_some() and
       * relying on the default implementation is enough.
       *
       * @param info       the info about the server we want to handshake with
       * @param callbacks  callbacks to be used for session policy decisions
       * @param policy     policy to be used for session policy decisions
       * @return           a list of usable sessions that might be empty if no
       *                   such session exists or passed the policy validation
       */
      virtual std::vector<Session_with_Handle> find(const Server_Information& info,
                                                    Callbacks& callbacks,
                                                    const Policy& policy);

      /**
       * Remove a specific session from the cache, if it exists.
       * The handle might contain either a session ID or a ticket.
       *
       * @param handle a Session_Handle of the session to be removed
       * @return the number of sessions that were removed
       */
      virtual size_t remove(const Session_Handle& handle) = 0;

      /**
       * Remove all sessions from the cache
       * @return the number of sessions that were removed
       */
      virtual size_t remove_all() = 0;

      /**
       * Declares whether the given Session_Manager implementation may emit
       * session tickets. Note that this _does not_ mean that the implementation
       * must always emit tickets.
       *
       * Concrete implementations should declare this, to allow the TLS
       * implementations to act accordingly. E.g. to advertise support for
       * session tickets in their Server Hello.
       *
       * @return true if the Session_Manager produces session tickets
       */
      virtual bool emits_session_tickets() { return false; }

      virtual ~Session_Manager() = default;

   protected:
      /**
       * @brief Internal retrieval function for a single session
       *
       * Try to obtain a Session from a Session_Handle that contains either
       * a session ID or a session ticket. This method should not apply any
       * policy decision (such as ticket expiry) but simply be a storage
       * interface.
       *
       * Applications that wish to implement their own Session_Manager will
       * have to provide an implementation for it.
       *
       * This method is called only by servers.
       *
       * @param handle a Session_Handle containing either an ID or a ticket
       * @return the obtained session or std::nullopt if none can be obtained
       */
      virtual std::optional<Session> retrieve_one(const Session_Handle& handle) = 0;

      /**
       * @brief Internal retrieval function to find sessions to resume
       *
       * Try to find saved sessions using info about the server we're planning
       * to connect to. It should return a list of sessions in preference order
       * of the session manager.
       *
       * Applications that wish to implement their own Session_Manager will
       * have to provide an implementation for it.
       *
       * Note that the TLS client implementations do not perform any checks on
       * the validity of the session for a given @p info. Particularly, it is
       * the Session_Manager's responsibility to ensure the restrictions posed
       * in RFC 8446 4.6.1 regarding server certificate validity for the given
       * @p info.
       *
       * This is called for TLS clients only.
       *
       * @param info               the information about the server
       * @param max_sessions_hint  a non-binding guideline for an upper bound of
       *                           sessions to return from this method
       *                           (will be at least 1 but potentially more)
       * @return the found sessions along with their handles (containing either a
       *         session ID or a ticket)
       */
      virtual std::vector<Session_with_Handle> find_some(const Server_Information& info, size_t max_sessions_hint) = 0;

      /**
       * Returns the base class' recursive mutex for reuse in derived classes
       */
      recursive_mutex_type& mutex() { return m_mutex; }

   private:
      std::vector<Session_with_Handle> find_and_filter(const Server_Information& info,
                                                       Callbacks& callbacks,
                                                       const Policy& policy);

   protected:
      std::shared_ptr<RandomNumberGenerator> m_rng;

   private:
      recursive_mutex_type m_mutex;
};

}  // namespace Botan::TLS

#endif
