/**
 * TLS Stateless Session Manager for stateless servers
 * (C) 2023 Jack Lloyd
 *     2023 René Meusel - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_TLS_SESSION_MANAGER_STATELESS_H_
#define BOTAN_TLS_SESSION_MANAGER_STATELESS_H_

#include <botan/tls_session_manager.h>

namespace Botan {

class RandomNumberGenerator;
class Credentials_Manager;

namespace TLS {

/**
 * A Session_Manager that emits Session_Handle objects with a Session_Ticket.
 *
 * This is useful for servers that do not want to hold any state about resumable
 * sessions. Using this implementation in a TLS client won't make sense.
 *
 * Returned Session_Handle objects won't contain a Session_ID. Retrieving
 * sessions via Session_ID will never return a session. Neither will searching
 * sessions by server information yield any result.
 */
class BOTAN_PUBLIC_API(3, 0) Session_Manager_Stateless : public Session_Manager {
   public:
      /**
       * The key to encrypt and authenticate session information will be drawn
       * from @p credentials_manager as `psk("tls-server", "session-ticket")`.
       * It is the responsibility of the calling application to set up its own
       * Credentials_Manager to provide a suitable key for this purpose.
       */
      Session_Manager_Stateless(const std::shared_ptr<Credentials_Manager>& credentials_manager,
                                const std::shared_ptr<RandomNumberGenerator>& rng);

      std::optional<Session_Handle> establish(const Session& session,
                                              const std::optional<Session_ID>& id = std::nullopt,
                                              bool tls12_no_ticket = false) override;

      void store(const Session& session, const Session_Handle& handle) override;

      size_t remove(const Session_Handle&) override { return 0; }

      size_t remove_all() override { return 0; }

      bool emits_session_tickets() override;

   protected:
      std::optional<Session> retrieve_one(const Session_Handle& handle) override;

      std::vector<Session_with_Handle> find_some(const Server_Information&, const size_t) override { return {}; }

   private:
      std::optional<SymmetricKey> get_ticket_key() noexcept;

   private:
      std::shared_ptr<Credentials_Manager> m_credentials_manager;
};

}  // namespace TLS

}  // namespace Botan

#endif
