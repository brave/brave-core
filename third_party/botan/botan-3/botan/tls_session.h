/*
* TLS Session
* (C) 2011-2012,2015 Jack Lloyd
* (C) 2022 René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SESSION_STATE_H_
#define BOTAN_TLS_SESSION_STATE_H_

#include <botan/secmem.h>
#include <botan/strong_type.h>
#include <botan/symkey.h>
#include <botan/tls_ciphersuite.h>
#include <botan/tls_magic.h>
#include <botan/tls_server_info.h>
#include <botan/tls_version.h>
#include <botan/x509cert.h>

#include <algorithm>
#include <chrono>
#include <span>
#include <variant>

namespace Botan::TLS {

// Different flavors of session handles are used, depending on the usage
// scenario and the TLS protocol version.

/// @brief holds a TLS 1.2 session ID for stateful resumption
using Session_ID = Strong<std::vector<uint8_t>, struct Session_ID_>;

/// @brief holds a TLS 1.2 session ticket for stateless resumption
using Session_Ticket = Strong<std::vector<uint8_t>, struct Session_Ticket_>;

/// @brief holds an opaque session handle as used in TLS 1.3 that could be
///        either a ticket for stateless resumption or a database handle.
using Opaque_Session_Handle = Strong<std::vector<uint8_t>, struct Opaque_Session_Handle_>;

inline auto operator<(const Session_ID& id1, const Session_ID& id2) {
   // TODO: C++20 better use std::lexicographical_compare_three_way
   //       that was not available on all target platforms at the time
   //       of this writing.
   return std::lexicographical_compare(id1.begin(), id1.end(), id2.begin(), id2.end());
}

/**
 * @brief Helper class to embody a session handle in all protocol versions
 *
 * Sessions in TLS 1.2 are identified by an arbitrary and unique ID of up to
 * 32 bytes or by a self-contained arbitrary-length ticket (RFC 5077).
 *
 * TLS 1.3 does not distinct between the two and handles both as tickets. Also
 * a TLS 1.3 server can issue multiple tickets in one connection and the
 * resumption mechanism is compatible with the PSK establishment.
 *
 * Concrete implementations of Session_Manager use this helper to distinguish
 * the different states and manage sessions for TLS 1.2 and 1.3 connections.
 *
 * Note that all information stored in a Session_Handle might be transmitted in
 * unprotected form. Hence, it should not contain any confidential information.
 */
class BOTAN_PUBLIC_API(3, 0) Session_Handle {
   public:
      /**
       * Constructs a Session_Handle from a session ID which is an
       * arbitrary byte vector that must be 32 bytes long at most.
       */
      Session_Handle(Session_ID id) : m_handle(std::move(id)) { validate_constraints(); }

      /**
       * Constructs a Session_Handle from a session ticket which is a
       * non-empty byte vector that must be 64kB long at most.
       * Typically, tickets facilitate stateless server implementations
       * and contain all relevant context in encrypted/authenticated form.
       *
       * Note that (for technical reasons) we enforce that tickets are
       * longer than 32 bytes.
       */
      Session_Handle(Session_Ticket ticket) : m_handle(std::move(ticket)) { validate_constraints(); }

      /**
       * Constructs a Session_Handle from an Opaque_Handle such as TLS 1.3
       * uses them in its resumption mechanism. This could be either a
       * Session_ID or a Session_Ticket and it is up to the Session_Manager
       * to figure out what it actually is.
       */
      Session_Handle(Opaque_Session_Handle ticket) : m_handle(std::move(ticket)) { validate_constraints(); }

      bool is_id() const { return std::holds_alternative<Session_ID>(m_handle); }

      bool is_ticket() const { return std::holds_alternative<Session_Ticket>(m_handle); }

      bool is_opaque_handle() const { return std::holds_alternative<Opaque_Session_Handle>(m_handle); }

      /**
       * Returns the Session_Handle as an opaque handle. If the object was not
       * constructed as an Opaque_Session_Handle, the contained value is
       * converted.
       */
      Opaque_Session_Handle opaque_handle() const;

      /**
       * If the Session_Handle was constructed with a Session_ID or an
       * Opaque_Session_Handle that can be converted to a Session_ID (up to
       * 32 bytes long), this returns the handle as a Session_ID. Otherwise,
       * std::nullopt is returned.
       */
      std::optional<Session_ID> id() const;

      /**
       * If the Session_Handle was constructed with a Session_Ticket or an
       * Opaque_Session_Handle this returns the handle as a Session_ID.
       * Otherwise, std::nullopt is returned.
       */
      std::optional<Session_Ticket> ticket() const;

      decltype(auto) get() const { return m_handle; }

   private:
      void validate_constraints() const;

   private:
      std::variant<Session_ID, Session_Ticket, Opaque_Session_Handle> m_handle;
};

class Client_Hello_13;
class Server_Hello_13;
class Callbacks;

/**
 * Represents basic information about a session that can be both
 * persisted for resumption and presented to the application as
 * a summary of a specific just-established TLS session.
 */
class BOTAN_PUBLIC_API(3, 0) Session_Base {
   public:
      Session_Base(std::chrono::system_clock::time_point start_time,
                   Protocol_Version version,
                   uint16_t ciphersuite,
                   Connection_Side connection_side,
                   uint16_t srtp_profile,
                   bool extended_master_secret,
                   bool encrypt_then_mac,
                   std::vector<X509_Certificate> peer_certs,
                   std::shared_ptr<const Public_Key> peer_raw_public_key,
                   Server_Information server_info) :
            m_start_time(start_time),
            m_version(version),
            m_ciphersuite(ciphersuite),
            m_connection_side(connection_side),
            m_srtp_profile(srtp_profile),
            m_extended_master_secret(extended_master_secret),
            m_encrypt_then_mac(encrypt_then_mac),
            m_peer_certs(std::move(peer_certs)),
            m_peer_raw_public_key(std::move(peer_raw_public_key)),
            m_server_info(std::move(server_info)) {}

   protected:
      Session_Base() = default;

   public:
      /**
       * Get the wall clock time this session began
       */
      std::chrono::system_clock::time_point start_time() const { return m_start_time; }

      /**
       * Get the negotiated protocol version of the TLS session
       */
      Protocol_Version version() const { return m_version; }

      /**
       * Get the ciphersuite code of the negotiated TLS session
       */
      uint16_t ciphersuite_code() const { return m_ciphersuite; }

      /**
       * Get the ciphersuite info of the negotiated TLS session
       */
      Ciphersuite ciphersuite() const;

      /**
       * Get which side of the connection we are/were acting as.
       */
      Connection_Side side() const { return m_connection_side; }

      /**
       * Get the negotiated DTLS-SRTP algorithm (RFC 5764)
       */
      uint16_t dtls_srtp_profile() const { return m_srtp_profile; }

      /**
       * Returns true if a TLS 1.2 session negotiated "encrypt then MAC";
       * TLS 1.3 sessions will always return false as they always use an AEAD.
       */
      bool supports_encrypt_then_mac() const { return m_encrypt_then_mac; }

      /**
       * Returns true if a TLS 1.2 session negotiated "extended master secret";
       * TLS 1.3 sessions will always return true (see RFC 8446 Appendix D).
       */
      bool supports_extended_master_secret() const { return m_extended_master_secret; }

      /**
       * Return the certificate chain of the peer (possibly empty)
       */
      const std::vector<X509_Certificate>& peer_certs() const { return m_peer_certs; }

      /**
       * Return the raw public key of the peer (possibly empty)
       */
      std::shared_ptr<const Public_Key> peer_raw_public_key() const { return m_peer_raw_public_key; }

      /**
       * Get information about the TLS server
       */
      const Server_Information& server_info() const { return m_server_info; }

   protected:
      std::chrono::system_clock::time_point m_start_time;

      Protocol_Version m_version;
      uint16_t m_ciphersuite;
      Connection_Side m_connection_side;
      uint16_t m_srtp_profile;

      bool m_extended_master_secret;
      bool m_encrypt_then_mac;

      std::vector<X509_Certificate> m_peer_certs;
      std::shared_ptr<const Public_Key> m_peer_raw_public_key;
      Server_Information m_server_info;
};

/**
 * Summarizes the negotiated features after a TLS handshake. Applications may
 * query those in Callbacks::tls_session_established().
 */
class BOTAN_PUBLIC_API(3, 0) Session_Summary : public Session_Base {
   public:
      /**
       * The Session_ID negotiated during the handshake.
       * Note that this does not carry any meaning in TLS 1.3 and might even
       * be empty.
       */
      const Session_ID& session_id() const { return m_session_id; }

      /**
       * The session ticket a TLS 1.2 server issued for this session.
       * Note that this may be set in TLS 1.2 clients only. It is _not_ the
       * ticket used to establish this session.
       */
      const std::optional<Session_Ticket>& session_ticket() const { return m_session_ticket; }

      /**
       * The negotiated identity of an externally provided preshared key used to
       * establish this session. For TLS 1.3 this may be any of the externally
       * provided PSKs offered by the client. PSK identities used as session
       * tickets for TLS 1.3 session resumption won't be shown here.
       */
      const std::optional<std::string>& external_psk_identity() const { return m_external_psk_identity; }

      /**
       * Indicates that the session was established using an externally provided
       * PSK. Session resumptions in TLS 1.3 (while technically implemented
       * using a PSK) are not considered here. @sa was_resumption()
       *
       * @note Botan 3.0 and 3.1 did incorrectly report true for session resumption.
       *
       * @returns true if the session was established using an externally
       *          provided PSK.
       */
      bool psk_used() const { return m_external_psk_identity.has_value(); }

      /**
       * Indicates that the session was resumed from a previous handshake state.
       *
       * @returns true if this session is a resumption, otherwise false
       */
      bool was_resumption() const { return m_was_resumption; }

      std::string kex_algo() const { return m_kex_algo; }

      std::string cipher_algo() const { return ciphersuite().cipher_algo(); }

      std::string mac_algo() const { return ciphersuite().mac_algo(); }

      std::string prf_algo() const { return ciphersuite().prf_algo(); }

   private:
      friend class Server_Impl_12;
      friend class Server_Impl_13;
      friend class Client_Impl_12;
      friend class Client_Impl_13;

      Session_Summary(const Session_Base& base, bool was_resumption, std::optional<std::string> psk_identity);

#if defined(BOTAN_HAS_TLS_13)
      Session_Summary(const Server_Hello_13& server_hello,
                      Connection_Side side,
                      std::vector<X509_Certificate> peer_certs,
                      std::shared_ptr<const Public_Key> peer_raw_public_key,
                      std::optional<std::string> psk_identity,
                      bool session_was_resumed,
                      Server_Information server_info,
                      std::chrono::system_clock::time_point current_timestamp);
#endif

      void set_session_id(Session_ID id) { m_session_id = std::move(id); }

      void set_session_ticket(Session_Ticket ticket) { m_session_ticket = std::move(ticket); }

   private:
      Session_ID m_session_id;
      std::optional<Session_Ticket> m_session_ticket;
      std::optional<std::string> m_external_psk_identity;

      bool m_was_resumption;
      std::string m_kex_algo;
};

/**
 * Represents a session's negotiated features along with all resumption
 * information to re-establish a TLS connection later on.
 */
class BOTAN_PUBLIC_API(3, 0) Session final : public Session_Base {
   public:
      /**
      * New TLS 1.2 session (sets session start time)
      */
      Session(const secure_vector<uint8_t>& master_secret,
              Protocol_Version version,
              uint16_t ciphersuite,
              Connection_Side side,
              bool supports_extended_master_secret,
              bool supports_encrypt_then_mac,
              const std::vector<X509_Certificate>& peer_certs,
              const Server_Information& server_info,
              uint16_t srtp_profile,
              std::chrono::system_clock::time_point current_timestamp,
              std::chrono::seconds lifetime_hint = std::chrono::seconds::max());

#if defined(BOTAN_HAS_TLS_13)

      /**
      * New TLS 1.3 session (sets session start time)
      */
      Session(const secure_vector<uint8_t>& session_psk,
              const std::optional<uint32_t>& max_early_data_bytes,
              uint32_t ticket_age_add,
              std::chrono::seconds lifetime_hint,
              Protocol_Version version,
              uint16_t ciphersuite,
              Connection_Side side,
              const std::vector<X509_Certificate>& peer_certs,
              std::shared_ptr<const Public_Key> peer_raw_public_key,
              const Server_Information& server_info,
              std::chrono::system_clock::time_point current_timestamp);

      /**
       * Create a new TLS 1.3 session object from server data structures
       * after a successful handshake with a TLS 1.3 client
       */
      Session(secure_vector<uint8_t>&& session_psk,
              const std::optional<uint32_t>& max_early_data_bytes,
              std::chrono::seconds lifetime_hint,
              const std::vector<X509_Certificate>& peer_certs,
              std::shared_ptr<const Public_Key> peer_raw_public_key,
              const Client_Hello_13& client_hello,
              const Server_Hello_13& server_hello,
              Callbacks& callbacks,
              RandomNumberGenerator& rng);

#endif

      /**
      * Load a session from DER representation (created by DER_encode)
      * @param ber_data DER representation buffer
      */
      Session(std::span<const uint8_t> ber_data);

      /**
      * Load a session from PEM representation (created by PEM_encode)
      * @param pem PEM representation
      */
      explicit Session(std::string_view pem);

      /**
      * Encode this session data for storage
      * @warning if the master secret is compromised so is the
      * session traffic
      */
      secure_vector<uint8_t> DER_encode() const;

      /**
      * Encrypt a session (useful for serialization or session tickets)
      */
      std::vector<uint8_t> encrypt(const SymmetricKey& key, RandomNumberGenerator& rng) const;

      /**
      * Decrypt a session created by encrypt
      * @param ctext the ciphertext returned by encrypt
      * @param ctext_size the size of ctext in bytes
      * @param key the same key used by the encrypting side
      */
      static inline Session decrypt(const uint8_t ctext[], size_t ctext_size, const SymmetricKey& key) {
         return Session::decrypt(std::span(ctext, ctext_size), key);
      }

      /**
      * Decrypt a session created by encrypt
      * @param ctext the ciphertext returned by encrypt
      * @param key the same key used by the encrypting side
      */
      static Session decrypt(std::span<const uint8_t> ctext, const SymmetricKey& key);

      /**
      * Encode this session data for storage
      * @warning if the master secret is compromised so is the
      * session traffic
      */
      std::string PEM_encode() const;

      /**
      * Get a reference to the contained master secret
      */
      const secure_vector<uint8_t>& master_secret() const { return m_master_secret; }

      /**
      * Get the contained master secret as a moved-out object
      */
      secure_vector<uint8_t> extract_master_secret();

      /**
       * Get whether the saved session supports sending/receiving of early data
       */
      bool supports_early_data() const { return m_early_data_allowed; }

      /**
      * Return the ticket obfuscation adder
      */
      uint32_t session_age_add() const { return m_ticket_age_add; }

      /**
      * Return the number of bytes allowed for 0-RTT early data
      */
      uint32_t max_early_data_bytes() const { return m_max_early_data_bytes; }

      /**
      * @return the lifetime of the ticket as defined by the TLS server
      */
      std::chrono::seconds lifetime_hint() const { return m_lifetime_hint; }

   private:
      // Struct Version history
      //
      // 20160812 - Pre TLS 1.3
      // 20220505 - Introduction of TLS 1.3 sessions
      //            - added fields:
      //              - m_early_data_allowed
      //              - m_max_early_data_bytes
      //              - m_ticket_age_add
      //              - m_lifetime_hint
      // 20230112 - Remove Session_ID and Session_Ticket from this object
      //            (association is now in the hands of the Session_Manager)
      //          - Peer certificates are now stored as a SEQUENCE
      // 20230222 - Remove deprecated and unused fields
      //            - compression method (always 0)
      //            - fragment size (always 0)
      //            - SRP identifier (always "")
      // 20231031 - Allow storage of peer's raw public key
      enum { TLS_SESSION_PARAM_STRUCT_VERSION = 20231031 };

      secure_vector<uint8_t> m_master_secret;

      bool m_early_data_allowed;
      uint32_t m_max_early_data_bytes;
      uint32_t m_ticket_age_add;
      std::chrono::seconds m_lifetime_hint;
};

/**
 * Helper struct to conveniently pass a Session and its Session_Handle around
 */
struct BOTAN_PUBLIC_API(3, 0) Session_with_Handle {
      Session session;
      Session_Handle handle;
};

}  // namespace Botan::TLS

#endif
