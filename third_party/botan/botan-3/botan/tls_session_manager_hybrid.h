/**
 * Hybrid Session Manager emitting both Tickets and storing sessions in Memory
 * (C) 2023 Jack Lloyd
 * (C) 2023 René Meusel - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_TLS_SESSION_MANAGER_HYBRID_H_
#define BOTAN_TLS_SESSION_MANAGER_HYBRID_H_

#include <botan/tls_session_manager.h>
#include <botan/tls_session_manager_stateless.h>

#include <memory>

namespace Botan {

class RandomNumberGenerator;

namespace TLS {

/**
 * A combination of the Session_Manager_Stateless and an arbitrary stateful
 * Session_Manager.
 *
 * This extends any stateful session manager to provide TLS 1.2 session ticket
 * support. Session_Handle objects may either be a Session_Ticket or Session_ID
 * when working with TLS 1.2 servers and depending on the peer's capability to
 * support session tickets.
 *
 * For TLS 1.3 sessions it will provide one of both, depending on the preference
 * defined in the class' constructor.
 *
 * For applications that implement a TLS server that allows handshakes with both
 * TLS 1.2 and TLS 1.3 clients, this is typically a good default option. Combine
 * it with the Session_Manager_SQLite or Session_Manager_In_Memory as needed.
 */
class BOTAN_PUBLIC_API(3, 0) Session_Manager_Hybrid final : public Session_Manager {
   public:
      /**
       * @param stateful_manager the underlying stateful manager instance
       *                         as a non-owning reference
       * @param credentials_manager the credentials manager to take the ticket
       *                            key in the stateless memory manager from
       * @param rng a RNG used for generating session key and for
       *        session encryption
       * @param prefer_tickets for TLS 1.3 connections, servers need to choose
       *                       whether to go for self-contained tickets or
       *                       short database handles
       */
      Session_Manager_Hybrid(std::unique_ptr<Session_Manager> stateful_manager,
                             const std::shared_ptr<Credentials_Manager>& credentials_manager,
                             const std::shared_ptr<RandomNumberGenerator>& rng,
                             bool prefer_tickets = true);

      std::optional<Session_Handle> establish(const Session& session,
                                              const std::optional<Session_ID>& id = std::nullopt,
                                              bool tls12_no_ticket = false) override;

      std::optional<Session> retrieve(const Session_Handle& handle,
                                      Callbacks& callbacks,
                                      const Policy& policy) override;

      std::vector<Session_with_Handle> find(const Server_Information& info,
                                            Callbacks& callbacks,
                                            const Policy& policy) override {
         return m_stateful->find(info, callbacks, policy);
      }

      void store(const Session& session, const Session_Handle& handle) override { m_stateful->store(session, handle); }

      size_t remove(const Session_Handle& handle) override { return m_stateful->remove(handle); }

      size_t remove_all() override { return m_stateful->remove_all(); }

      bool emits_session_tickets() override;

      Session_Manager* underlying_stateful_manager() { return m_stateful.get(); }

   protected:
      // The Hybrid_Session_Manager just delegates to its underlying managers
      // via the public retrieval API. Its own "storage interface" is therefore
      // never called.
      std::optional<Session> retrieve_one(const Session_Handle&) override {
         BOTAN_ASSERT(false, "This should never be called");
      }

      std::vector<Session_with_Handle> find_some(const Server_Information&, const size_t) override {
         BOTAN_ASSERT(false, "This should never be called");
      }

   private:
      std::unique_ptr<Session_Manager> m_stateful;
      Session_Manager_Stateless m_stateless;

      bool m_prefer_tickets;
};

}  // namespace TLS

}  // namespace Botan

#endif
