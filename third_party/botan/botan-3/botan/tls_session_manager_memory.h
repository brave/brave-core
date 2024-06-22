/**
 * TLS Session Manager in Memory
 * (C) 2011 Jack Lloyd
 * (C) 2023 René Meusel - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_TLS_SESSION_MANAGER_IN_MEMORY_H_
#define BOTAN_TLS_SESSION_MANAGER_IN_MEMORY_H_

#include <botan/mutex.h>
#include <botan/tls_session.h>
#include <botan/tls_session_manager.h>

#include <deque>
#include <map>

namespace Botan {

class RandomNumberGenerator;

namespace TLS {

/**
 * A thread-safe Session_Manager that stores TLS sessions in memory.
 *
 * The Session_Handle objects emitted by this manager when establishing a new
 * session (i.e in the TLS server) will never contain a Session_Ticket but only a
 * Session_ID. Storing received sessions (i.e. in the TLS client) under either
 * a Session_ID or a Session_Ticket will however echo them back.
 *
 * In other words, this manager _will_ support ticket-based resumption in a
 * TLS client but it won't issue tickets on a TLS server.
 *
 * For applications that implement a TLS client and that do not want to persist
 * sessions to non-volatile memory, this is typically a good default option.
 */
class BOTAN_PUBLIC_API(3, 0) Session_Manager_In_Memory : public Session_Manager {
   public:
      /**
       * @param rng a RNG used for generating session key and for
       *        session encryption
       * @param max_sessions a hint on the maximum number of sessions
       *        to keep in memory at any one time. (If zero, don't cap)
       */
      Session_Manager_In_Memory(const std::shared_ptr<RandomNumberGenerator>& rng, size_t max_sessions = 1000);

      void store(const Session& session, const Session_Handle& handle) override;
      size_t remove(const Session_Handle& handle) override;
      size_t remove_all() override;

      size_t capacity() const { return m_max_sessions; }

      bool emits_session_tickets() override { return false; }

   protected:
      std::optional<Session> retrieve_one(const Session_Handle& handle) override;
      std::vector<Session_with_Handle> find_some(const Server_Information& info, size_t max_sessions_hint) override;

   private:
      size_t remove_internal(const Session_Handle& handle);

   private:
      size_t m_max_sessions;

      std::map<Session_ID, Session_with_Handle> m_sessions;
      std::optional<std::deque<Session_ID>> m_fifo;
};

}  // namespace TLS

}  // namespace Botan

#endif
