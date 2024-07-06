/**
 * Wrapper type for a TLS 1.3 session ticket
 * (C) 2023 Jack Lloyd
 *     2023 René Meusel - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_TLS_13_TICKET_H_
#define BOTAN_TLS_13_TICKET_H_

#include <botan/tls_external_psk.h>
#include <botan/tls_session.h>
#include <botan/types.h>

#include <chrono>
#include <cstdint>
#include <vector>

namespace Botan::TLS {

/// @brief holds a PSK identity as used in TLS 1.3
using PresharedKeyID = Strong<std::string, struct PresharedKeyID_>;

/**
 * Represents a TLS 1.3 PSK identity as found in the Preshared Key extension
 * with an opaque identity and an associated (obfuscated) ticket age. The latter
 * is not applicable for externally provided PSKs.
 */
class BOTAN_PUBLIC_API(3, 1) PskIdentity {
   public:
      /**
       * Construct from information provided in the peer's ClientHello
       */
      PskIdentity(std::vector<uint8_t> identity, const uint32_t obfuscated_age) :
            m_identity(std::move(identity)), m_obfuscated_age(obfuscated_age) {}

      /**
       * Construct from a session stored by the client
       */
      PskIdentity(Opaque_Session_Handle identity, std::chrono::milliseconds age, uint32_t ticket_age_add);

      /**
       * Construct from an externally provided PSK in the client
       */
      PskIdentity(PresharedKeyID identity);

      const std::vector<uint8_t>& identity() const { return m_identity; }

      std::string identity_as_string() const;

      /**
       * If this represents a PSK for session resumption, it returns the
       * session's age given the de-obfuscation parameter @p ticket_age_add. For
       * externally provided PSKs this method does not provide any meaningful
       * information.
       */
      std::chrono::milliseconds age(uint32_t ticket_age_add) const;

      uint32_t obfuscated_age() const { return m_obfuscated_age; }

   private:
      std::vector<uint8_t> m_identity;
      uint32_t m_obfuscated_age;
};

/**
 * Botan 3.0.0 used the class name "Ticket". In Botan 3.1.0 we decided to
 * re-name it to the more generic term "PskIdentity" to better reflect its dual
 * use case for resumption and externally provided PSKs.
 */
BOTAN_DEPRECATED("Use PskIdentity") typedef PskIdentity Ticket;

}  // namespace Botan::TLS

#endif
