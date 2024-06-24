/*
* TLS Session Manager Noop
* (C) 2011 Jack Lloyd
*     2023 René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SESSION_MANAGER_NOOP_H_
#define BOTAN_TLS_SESSION_MANAGER_NOOP_H_

#include <botan/tls_session_manager.h>

namespace Botan::TLS {

/**
 * An implementation of Session_Manager that does not save sessions at all,
 * preventing session resumption.
 *
 * For applications that do not want to support session resumption at all,
 * this is typically a good choice.
 */
class BOTAN_PUBLIC_API(3, 0) Session_Manager_Noop final : public Session_Manager {
   public:
      Session_Manager_Noop();

      std::optional<Session_Handle> establish(const Session&,
                                              const std::optional<Session_ID>& = std::nullopt,
                                              bool = false) override {
         return std::nullopt;
      }

      void store(const Session&, const Session_Handle&) override {}

      size_t remove(const Session_Handle&) override { return 0; }

      size_t remove_all() override { return 0; }

   protected:
      std::optional<Session> retrieve_one(const Session_Handle&) override { return std::nullopt; }

      std::vector<Session_with_Handle> find_some(const Server_Information&, const size_t) override { return {}; }
};

}  // namespace Botan::TLS

#endif
