/*
* TLS Messages
* (C) 2004-2011,2015 Jack Lloyd
*     2016 Matthias Gierlings
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_MESSAGES_H_
#define BOTAN_TLS_MESSAGES_H_

#include <chrono>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <botan/pk_keys.h>
#include <botan/strong_type.h>
#include <botan/tls_ciphersuite.h>
#include <botan/tls_extensions.h>
#include <botan/tls_handshake_msg.h>
#include <botan/tls_policy.h>
#include <botan/tls_session.h>
#include <botan/x509cert.h>

namespace Botan {

class Public_Key;
class Credentials_Manager;

namespace OCSP {
class Response;
}

namespace TLS {

class Session_Manager;
class Handshake_IO;
class Handshake_State;
class Hello_Retry_Request;
class Callbacks;
class Cipher_State;
class Policy;

std::vector<uint8_t> make_hello_random(RandomNumberGenerator& rng, Callbacks& cb, const Policy& policy);

/**
* DTLS Hello Verify Request
*/
class BOTAN_UNSTABLE_API Hello_Verify_Request final : public Handshake_Message {
   public:
      std::vector<uint8_t> serialize() const override;

      Handshake_Type type() const override { return Handshake_Type::HelloVerifyRequest; }

      const std::vector<uint8_t>& cookie() const { return m_cookie; }

      explicit Hello_Verify_Request(const std::vector<uint8_t>& buf);

      Hello_Verify_Request(const std::vector<uint8_t>& client_hello_bits,
                           std::string_view client_identity,
                           const SymmetricKey& secret_key);

   private:
      std::vector<uint8_t> m_cookie;
};

class Client_Hello_Internal;

/**
* Client Hello Message
*/
class BOTAN_UNSTABLE_API Client_Hello : public Handshake_Message {
   public:
      Client_Hello(const Client_Hello&) = delete;
      Client_Hello& operator=(const Client_Hello&) = delete;
      Client_Hello(Client_Hello&&) noexcept;
      Client_Hello& operator=(Client_Hello&&) noexcept;

      ~Client_Hello() override;

      Handshake_Type type() const override;

      /**
       * Return the version indicated in the ClientHello.
       * This may differ from the version indicated in the supported_versions extension.
       *
       * See RFC 8446 4.1.2:
       *   TLS 1.3, the client indicates its version preferences in the
       *   "supported_versions" extension (Section 4.2.1) and the
       *   legacy_version field MUST be set to 0x0303, which is the version
       *   number for TLS 1.2.
       */
      Protocol_Version legacy_version() const;

      const std::vector<uint8_t>& random() const;

      const Session_ID& session_id() const;

      const std::vector<uint16_t>& ciphersuites() const;

      bool offered_suite(uint16_t ciphersuite) const;

      std::vector<Signature_Scheme> signature_schemes() const;
      std::vector<Signature_Scheme> certificate_signature_schemes() const;

      std::vector<Group_Params> supported_ecc_curves() const;

      // This returns any groups in the FFDHE range
      std::vector<Group_Params> supported_dh_groups() const;

      std::vector<Protocol_Version> supported_versions() const;

      std::string sni_hostname() const;

      bool supports_alpn() const;

      bool sent_signature_algorithms() const;

      std::vector<std::string> next_protocols() const;

      std::vector<uint16_t> srtp_profiles() const;

      std::vector<uint8_t> serialize() const override;

      const std::vector<uint8_t>& cookie() const;

      std::vector<uint8_t> cookie_input_data() const;

      std::set<Extension_Code> extension_types() const;

      const Extensions& extensions() const;

   protected:
      Client_Hello();
      explicit Client_Hello(std::unique_ptr<Client_Hello_Internal> data);

      const std::vector<uint8_t>& compression_methods() const;

   protected:
      std::unique_ptr<Client_Hello_Internal> m_data;
};

class BOTAN_UNSTABLE_API Client_Hello_12 final : public Client_Hello {
   public:
      class Settings final {
         public:
            Settings(const Protocol_Version version, std::string_view hostname = "") :
                  m_new_session_version(version), m_hostname(hostname) {}

            Protocol_Version protocol_version() const { return m_new_session_version; }

            const std::string& hostname() const { return m_hostname; }

         private:
            const Protocol_Version m_new_session_version;
            const std::string m_hostname;
      };

   public:
      explicit Client_Hello_12(const std::vector<uint8_t>& buf);

      Client_Hello_12(Handshake_IO& io,
                      Handshake_Hash& hash,
                      const Policy& policy,
                      Callbacks& cb,
                      RandomNumberGenerator& rng,
                      const std::vector<uint8_t>& reneg_info,
                      const Settings& client_settings,
                      const std::vector<std::string>& next_protocols);

      Client_Hello_12(Handshake_IO& io,
                      Handshake_Hash& hash,
                      const Policy& policy,
                      Callbacks& cb,
                      RandomNumberGenerator& rng,
                      const std::vector<uint8_t>& reneg_info,
                      const Session_with_Handle& session_and_handle,
                      const std::vector<std::string>& next_protocols);

   protected:
      friend class Client_Hello_13;  // to allow construction by Client_Hello_13::parse()
      Client_Hello_12(std::unique_ptr<Client_Hello_Internal> data);

   public:
      using Client_Hello::compression_methods;
      using Client_Hello::random;

      bool prefers_compressed_ec_points() const;

      bool secure_renegotiation() const;

      std::vector<uint8_t> renegotiation_info() const;

      bool supports_session_ticket() const;

      Session_Ticket session_ticket() const;

      std::optional<Session_Handle> session_handle() const;

      bool supports_extended_master_secret() const;

      bool supports_cert_status_message() const;

      bool supports_encrypt_then_mac() const;

      void update_hello_cookie(const Hello_Verify_Request& hello_verify);

   private:
      void add_tls12_supported_groups_extensions(const Policy& policy);
};

#if defined(BOTAN_HAS_TLS_13)

class BOTAN_UNSTABLE_API Client_Hello_13 final : public Client_Hello {
   public:
      /**
       * Creates a client hello which might optionally use the passed-in
       * @p session for resumption. In that case, this will "extract" the
       * master secret from the passed-in @p session.
       */
      Client_Hello_13(const Policy& policy,
                      Callbacks& cb,
                      RandomNumberGenerator& rng,
                      std::string_view hostname,
                      const std::vector<std::string>& next_protocols,
                      std::optional<Session_with_Handle>& session,
                      std::vector<ExternalPSK> psks);

      static std::variant<Client_Hello_13, Client_Hello_12> parse(const std::vector<uint8_t>& buf);

      void retry(const Hello_Retry_Request& hrr,
                 const Transcript_Hash_State& transcript_hash_state,
                 Callbacks& cb,
                 RandomNumberGenerator& rng);

      /**
       * Select the highest protocol version from the list of versions
       * supported by the client. If no such version can be determind this
       * returns std::nullopt.
       */
      std::optional<Protocol_Version> highest_supported_version(const Policy& policy) const;

      /**
       * This validates that a Client Hello received after sending a Hello
       * Retry Request was updated in accordance with RFC 8446 4.1.2. If issues
       * are found, this method throws accordingly.
       */
      void validate_updates(const Client_Hello_13& new_ch);

   private:
      Client_Hello_13(std::unique_ptr<Client_Hello_Internal> data);

      /**
       * If the Client Hello contains a PSK extensions with identities this will
       * generate the PSK binders as described in RFC 8446 4.2.11.2.
       * Note that the passed in \p transcript_hash_state might be virgin for
       * the initial Client Hello and should be primed with ClientHello1 and
       * HelloRetryRequest for an updated Client Hello.
       */
      void calculate_psk_binders(Transcript_Hash_State transcript_hash_state);
};

#endif  // BOTAN_HAS_TLS_13

class Server_Hello_Internal;

/**
* Server Hello Message
*/
class BOTAN_UNSTABLE_API Server_Hello : public Handshake_Message {
   public:
      Server_Hello(const Server_Hello&) = delete;
      Server_Hello& operator=(const Server_Hello&) = delete;
      Server_Hello(Server_Hello&&) noexcept;
      Server_Hello& operator=(Server_Hello&&) noexcept;

      ~Server_Hello() override;

      std::vector<uint8_t> serialize() const override;

      Handshake_Type type() const override;

      // methods available in both subclasses' interface
      uint16_t ciphersuite() const;
      const Extensions& extensions() const;
      const Session_ID& session_id() const;

      virtual Protocol_Version selected_version() const = 0;

   protected:
      explicit Server_Hello(std::unique_ptr<Server_Hello_Internal> data);

      // methods used internally and potentially exposed by one of the subclasses
      std::set<Extension_Code> extension_types() const;
      const std::vector<uint8_t>& random() const;
      uint8_t compression_method() const;
      Protocol_Version legacy_version() const;

   protected:
      std::unique_ptr<Server_Hello_Internal> m_data;
};

class BOTAN_UNSTABLE_API Server_Hello_12 final : public Server_Hello {
   public:
      class Settings final {
         public:
            Settings(Session_ID new_session_id,
                     Protocol_Version new_session_version,
                     uint16_t ciphersuite,
                     bool offer_session_ticket) :
                  m_new_session_id(std::move(new_session_id)),
                  m_new_session_version(new_session_version),
                  m_ciphersuite(ciphersuite),
                  m_offer_session_ticket(offer_session_ticket) {}

            const Session_ID& session_id() const { return m_new_session_id; }

            Protocol_Version protocol_version() const { return m_new_session_version; }

            uint16_t ciphersuite() const { return m_ciphersuite; }

            bool offer_session_ticket() const { return m_offer_session_ticket; }

         private:
            const Session_ID m_new_session_id;
            Protocol_Version m_new_session_version;
            uint16_t m_ciphersuite;
            bool m_offer_session_ticket;
      };

      Server_Hello_12(Handshake_IO& io,
                      Handshake_Hash& hash,
                      const Policy& policy,
                      Callbacks& cb,
                      RandomNumberGenerator& rng,
                      const std::vector<uint8_t>& secure_reneg_info,
                      const Client_Hello_12& client_hello,
                      const Settings& settings,
                      std::string_view next_protocol);

      Server_Hello_12(Handshake_IO& io,
                      Handshake_Hash& hash,
                      const Policy& policy,
                      Callbacks& cb,
                      RandomNumberGenerator& rng,
                      const std::vector<uint8_t>& secure_reneg_info,
                      const Client_Hello_12& client_hello,
                      const Session& resumed_session,
                      bool offer_session_ticket,
                      std::string_view next_protocol);

      explicit Server_Hello_12(const std::vector<uint8_t>& buf);

   protected:
      friend class Server_Hello_13;  // to allow construction by Server_Hello_13::parse()
      explicit Server_Hello_12(std::unique_ptr<Server_Hello_Internal> data);

   public:
      using Server_Hello::compression_method;
      using Server_Hello::extension_types;
      using Server_Hello::legacy_version;
      using Server_Hello::random;

      /**
       * @returns the selected version as indicated in the legacy_version field
       */
      Protocol_Version selected_version() const override;

      bool secure_renegotiation() const;

      std::vector<uint8_t> renegotiation_info() const;

      std::string next_protocol() const;

      bool supports_extended_master_secret() const;

      bool supports_encrypt_then_mac() const;

      bool supports_certificate_status_message() const;

      bool supports_session_ticket() const;

      uint16_t srtp_profile() const;
      bool prefers_compressed_ec_points() const;

      /**
       * Return desired downgrade version indicated by hello random, if any.
       */
      std::optional<Protocol_Version> random_signals_downgrade() const;
};

#if defined(BOTAN_HAS_TLS_13)

class Hello_Retry_Request;

class BOTAN_UNSTABLE_API Server_Hello_13 : public Server_Hello {
   protected:
      static const struct Server_Hello_Tag {
      } as_server_hello;

      static const struct Hello_Retry_Request_Tag {
      } as_hello_retry_request;

      static const struct Hello_Retry_Request_Creation_Tag {
      } as_new_hello_retry_request;

      // These constructors are meant for instantiating Server Hellos
      // after parsing a peer's message. They perform basic validation
      // and are therefore not suitable for constructing a message to
      // be sent to a client.
      explicit Server_Hello_13(std::unique_ptr<Server_Hello_Internal> data, Server_Hello_Tag tag = as_server_hello);
      explicit Server_Hello_13(std::unique_ptr<Server_Hello_Internal> data, Hello_Retry_Request_Tag tag);
      void basic_validation() const;

      // Instantiate a Server Hello as response to a client's Client Hello
      // (called from Server_Hello_13::create())
      Server_Hello_13(const Client_Hello_13& ch,
                      std::optional<Named_Group> key_exchange_group,
                      Session_Manager& session_mgr,
                      Credentials_Manager& credentials_mgr,
                      RandomNumberGenerator& rng,
                      Callbacks& cb,
                      const Policy& policy);

      explicit Server_Hello_13(std::unique_ptr<Server_Hello_Internal> data, Hello_Retry_Request_Creation_Tag tag);

   public:
      static std::variant<Hello_Retry_Request, Server_Hello_13> create(const Client_Hello_13& ch,
                                                                       bool hello_retry_request_allowed,
                                                                       Session_Manager& session_mgr,
                                                                       Credentials_Manager& credentials_mgr,
                                                                       RandomNumberGenerator& rng,
                                                                       const Policy& policy,
                                                                       Callbacks& cb);

      static std::variant<Hello_Retry_Request, Server_Hello_13, Server_Hello_12> parse(const std::vector<uint8_t>& buf);

      /**
       * Return desired downgrade version indicated by hello random, if any.
       */
      std::optional<Protocol_Version> random_signals_downgrade() const;

      /**
       * @returns the selected version as indicated by the supported_versions extension
       */
      Protocol_Version selected_version() const final;
};

class BOTAN_UNSTABLE_API Hello_Retry_Request final : public Server_Hello_13 {
   protected:
      friend class Server_Hello_13;  // to allow construction by Server_Hello_13::parse() and ::create()
      explicit Hello_Retry_Request(std::unique_ptr<Server_Hello_Internal> data);
      Hello_Retry_Request(const Client_Hello_13& ch, Named_Group selected_group, const Policy& policy, Callbacks& cb);

   public:
      Handshake_Type type() const override { return Handshake_Type::HelloRetryRequest; }

      Handshake_Type wire_type() const override { return Handshake_Type::ServerHello; }
};

class BOTAN_UNSTABLE_API Encrypted_Extensions final : public Handshake_Message {
   public:
      explicit Encrypted_Extensions(const std::vector<uint8_t>& buf);
      Encrypted_Extensions(const Client_Hello_13& client_hello, const Policy& policy, Callbacks& cb);

      Handshake_Type type() const override { return Handshake_Type::EncryptedExtensions; }

      const Extensions& extensions() const { return m_extensions; }

      std::vector<uint8_t> serialize() const override;

   private:
      Extensions m_extensions;
};

#endif  // BOTAN_HAS_TLS_13

/**
* Client Key Exchange Message
*/
class BOTAN_UNSTABLE_API Client_Key_Exchange final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::ClientKeyExchange; }

      const secure_vector<uint8_t>& pre_master_secret() const { return m_pre_master; }

      /**
       * @returns the agreed upon PSK identity or std::nullopt if not applicable
       */
      const std::optional<std::string>& psk_identity() const { return m_psk_identity; }

      Client_Key_Exchange(Handshake_IO& io,
                          Handshake_State& state,
                          const Policy& policy,
                          Credentials_Manager& creds,
                          const Public_Key* server_public_key,
                          std::string_view hostname,
                          RandomNumberGenerator& rng);

      Client_Key_Exchange(const std::vector<uint8_t>& buf,
                          const Handshake_State& state,
                          const Private_Key* server_rsa_kex_key,
                          Credentials_Manager& creds,
                          const Policy& policy,
                          RandomNumberGenerator& rng);

   private:
      std::vector<uint8_t> serialize() const override { return m_key_material; }

      std::vector<uint8_t> m_key_material;
      secure_vector<uint8_t> m_pre_master;
      std::optional<std::string> m_psk_identity;
};

/**
* Certificate Message of TLS 1.2
*/
class BOTAN_UNSTABLE_API Certificate_12 final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::Certificate; }

      const std::vector<X509_Certificate>& cert_chain() const { return m_certs; }

      size_t count() const { return m_certs.size(); }

      bool empty() const { return m_certs.empty(); }

      Certificate_12(Handshake_IO& io, Handshake_Hash& hash, const std::vector<X509_Certificate>& certs);

      Certificate_12(const std::vector<uint8_t>& buf, const Policy& policy);

      std::vector<uint8_t> serialize() const override;

   private:
      std::vector<X509_Certificate> m_certs;
};

#if defined(BOTAN_HAS_TLS_13)

class Certificate_Request_13;

/**
* Certificate Message of TLS 1.3
*/
class BOTAN_UNSTABLE_API Certificate_13 final : public Handshake_Message {
   public:
      class Certificate_Entry {
         public:
            Certificate_Entry(TLS_Data_Reader& reader, const Connection_Side side, const Certificate_Type cert_type);
            Certificate_Entry(X509_Certificate cert);
            Certificate_Entry(std::shared_ptr<Public_Key> raw_public_key);

            bool has_certificate() const { return m_certificate.has_value(); }

            const X509_Certificate& certificate() const;
            std::shared_ptr<const Public_Key> public_key() const;

            std::vector<uint8_t> serialize() const;

            Extensions& extensions() { return m_extensions; }

            const Extensions& extensions() const { return m_extensions; }

         private:
            std::optional<X509_Certificate> m_certificate;
            std::shared_ptr<Public_Key> m_raw_public_key;
            Extensions m_extensions;
      };

   public:
      Handshake_Type type() const override { return Handshake_Type::Certificate; }

      std::vector<X509_Certificate> cert_chain() const;

      bool has_certificate_chain() const;
      bool is_raw_public_key() const;

      size_t count() const { return m_entries.size(); }

      bool empty() const { return m_entries.empty(); }

      std::shared_ptr<const Public_Key> public_key() const;
      const X509_Certificate& leaf() const;

      const std::vector<uint8_t>& request_context() const { return m_request_context; }

      /**
       * Create a Client Certificate message
       * ... in response to a Certificate Request message.
       */
      Certificate_13(const Certificate_Request_13& cert_request,
                     std::string_view hostname,
                     Credentials_Manager& credentials_manager,
                     Callbacks& callbacks,
                     Certificate_Type cert_type);

      /**
       * Create a Server Certificate message
       * ... in response to a Client Hello indicating the need to authenticate
       *     with a server certificate.
       */
      Certificate_13(const Client_Hello_13& client_hello,
                     Credentials_Manager& credentials_manager,
                     Callbacks& callbacks,
                     Certificate_Type cert_type);

      /**
      * Deserialize a Certificate message
      * @param buf the serialized message
      * @param policy the TLS policy
      * @param side is this a Connection_Side::Server or Connection_Side::Client certificate message
      * @param cert_type is the certificate type that was negotiated during the handshake
      */
      Certificate_13(const std::vector<uint8_t>& buf,
                     const Policy& policy,
                     Connection_Side side,
                     Certificate_Type cert_type);

      /**
      * Validate a Certificate message regarding what extensions are expected based on
      * previous handshake messages. Also call the tls_examine_extenions() callback
      * for each entry.
      *
      * @param requested_extensions Extensions of Client_Hello or Certificate_Request messages
      * @param cb Callback that will be called for each extension.
      */
      void validate_extensions(const std::set<Extension_Code>& requested_extensions, Callbacks& cb) const;

      /**
       * Verify the certificate chain
       *
       * @throws if verification fails.
       */
      void verify(Callbacks& callbacks,
                  const Policy& policy,
                  Credentials_Manager& creds,
                  std::string_view hostname,
                  bool use_ocsp) const;

      std::vector<uint8_t> serialize() const override;

   private:
      void setup_entries(std::vector<X509_Certificate> cert_chain,
                         const Certificate_Status_Request* csr,
                         Callbacks& callbacks);
      void setup_entry(std::shared_ptr<Public_Key> raw_public_key, Callbacks& callbacks);

      void verify_certificate_chain(Callbacks& callbacks,
                                    const Policy& policy,
                                    Credentials_Manager& creds,
                                    std::string_view hostname,
                                    bool use_ocsp,
                                    Usage_Type usage_type) const;

   private:
      std::vector<uint8_t> m_request_context;
      std::vector<Certificate_Entry> m_entries;
      Connection_Side m_side;
};

#endif  // BOTAN_HAS_TLS_13

/**
* Certificate Status (RFC 6066)
*/
class BOTAN_UNSTABLE_API Certificate_Status final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::CertificateStatus; }

      //std::shared_ptr<const OCSP::Response> response() const { return m_response; }

      const std::vector<uint8_t>& response() const { return m_response; }

      explicit Certificate_Status(const std::vector<uint8_t>& buf, Connection_Side from);

      Certificate_Status(Handshake_IO& io, Handshake_Hash& hash, const OCSP::Response& response);

      /*
       * Create a Certificate_Status message using an already DER encoded OCSP response.
       */
      Certificate_Status(Handshake_IO& io, Handshake_Hash& hash, std::vector<uint8_t> raw_response_bytes);

      Certificate_Status(std::vector<uint8_t> raw_response_bytes);

      std::vector<uint8_t> serialize() const override;

   private:
      std::vector<uint8_t> m_response;
};

/**
* Certificate Request Message (TLS 1.2)
*/
class BOTAN_UNSTABLE_API Certificate_Request_12 final : public Handshake_Message {
   public:
      Handshake_Type type() const override;

      const std::vector<std::string>& acceptable_cert_types() const;

      const std::vector<X509_DN>& acceptable_CAs() const;

      const std::vector<Signature_Scheme>& signature_schemes() const;

      Certificate_Request_12(Handshake_IO& io,
                             Handshake_Hash& hash,
                             const Policy& policy,
                             const std::vector<X509_DN>& allowed_cas);

      explicit Certificate_Request_12(const std::vector<uint8_t>& buf);

      std::vector<uint8_t> serialize() const override;

   private:
      std::vector<X509_DN> m_names;
      std::vector<std::string> m_cert_key_types;
      std::vector<Signature_Scheme> m_schemes;
};

#if defined(BOTAN_HAS_TLS_13)

class BOTAN_UNSTABLE_API Certificate_Request_13 final : public Handshake_Message {
   public:
      Handshake_Type type() const override;

      Certificate_Request_13(const std::vector<uint8_t>& buf, Connection_Side side);

      //! Creates a Certificate_Request message if it is required by the configuration
      //! @return std::nullopt if configuration does not require client authentication
      static std::optional<Certificate_Request_13> maybe_create(const Client_Hello_13& sni_hostname,
                                                                Credentials_Manager& cred_mgr,
                                                                Callbacks& callbacks,
                                                                const Policy& policy);

      std::vector<X509_DN> acceptable_CAs() const;
      const std::vector<Signature_Scheme>& signature_schemes() const;
      const std::vector<Signature_Scheme>& certificate_signature_schemes() const;

      const Extensions& extensions() const { return m_extensions; }

      std::vector<uint8_t> serialize() const override;

      const std::vector<uint8_t>& context() const { return m_context; }

   private:
      Certificate_Request_13(std::vector<X509_DN> acceptable_CAs, const Policy& policy, Callbacks& callbacks);

   private:
      std::vector<uint8_t> m_context;
      Extensions m_extensions;
};

#endif

class BOTAN_UNSTABLE_API Certificate_Verify : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::CertificateVerify; }

      Signature_Scheme signature_scheme() const { return m_scheme; }

      Certificate_Verify(const std::vector<uint8_t>& buf);
      Certificate_Verify() = default;

      std::vector<uint8_t> serialize() const override;

   protected:
      std::vector<uint8_t> m_signature;
      Signature_Scheme m_scheme;
};

/**
* Certificate Verify Message
*/
class BOTAN_UNSTABLE_API Certificate_Verify_12 final : public Certificate_Verify {
   public:
      using Certificate_Verify::Certificate_Verify;

      Certificate_Verify_12(Handshake_IO& io,
                            Handshake_State& state,
                            const Policy& policy,
                            RandomNumberGenerator& rng,
                            const Private_Key* key);

      /**
      * Check the signature on a certificate verify message
      * @param cert the purported certificate
      * @param state the handshake state
      * @param policy the TLS policy
      */
      bool verify(const X509_Certificate& cert, const Handshake_State& state, const Policy& policy) const;
};

#if defined(BOTAN_HAS_TLS_13)

/**
* Certificate Verify Message
*/
class BOTAN_UNSTABLE_API Certificate_Verify_13 final : public Certificate_Verify {
   public:
      /**
      * Deserialize a Certificate message
      * @param buf the serialized message
      * @param side is this a Connection_Side::Server or Connection_Side::Client certificate message
      */
      Certificate_Verify_13(const std::vector<uint8_t>& buf, Connection_Side side);

      Certificate_Verify_13(const Certificate_13& certificate_message,
                            const std::vector<Signature_Scheme>& peer_allowed_schemes,
                            std::string_view hostname,
                            const Transcript_Hash& hash,
                            Connection_Side whoami,
                            Credentials_Manager& creds_mgr,
                            const Policy& policy,
                            Callbacks& callbacks,
                            RandomNumberGenerator& rng);

      bool verify(const Public_Key& public_key, Callbacks& callbacks, const Transcript_Hash& transcript_hash) const;

   private:
      Connection_Side m_side;
};

#endif

/**
* Finished Message
*/
class BOTAN_UNSTABLE_API Finished : public Handshake_Message {
   public:
      explicit Finished(const std::vector<uint8_t>& buf);

      Handshake_Type type() const override { return Handshake_Type::Finished; }

      std::vector<uint8_t> verify_data() const;

      std::vector<uint8_t> serialize() const override;

   protected:
      using Handshake_Message::Handshake_Message;
      std::vector<uint8_t> m_verification_data;
};

class BOTAN_UNSTABLE_API Finished_12 final : public Finished {
   public:
      using Finished::Finished;
      Finished_12(Handshake_IO& io, Handshake_State& state, Connection_Side side);

      bool verify(const Handshake_State& state, Connection_Side side) const;
};

#if defined(BOTAN_HAS_TLS_13)
class BOTAN_UNSTABLE_API Finished_13 final : public Finished {
   public:
      using Finished::Finished;
      Finished_13(Cipher_State* cipher_state, const Transcript_Hash& transcript_hash);

      bool verify(Cipher_State* cipher_state, const Transcript_Hash& transcript_hash) const;
};
#endif

/**
* Hello Request Message
*/
class BOTAN_UNSTABLE_API Hello_Request final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::HelloRequest; }

      explicit Hello_Request(Handshake_IO& io);
      explicit Hello_Request(const std::vector<uint8_t>& buf);

   private:
      std::vector<uint8_t> serialize() const override;
};

/**
* Server Key Exchange Message
*/
class BOTAN_UNSTABLE_API Server_Key_Exchange final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::ServerKeyExchange; }

      const std::vector<uint8_t>& params() const { return m_params; }

      bool verify(const Public_Key& server_key, const Handshake_State& state, const Policy& policy) const;

      // Only valid for certain kex types
      const PK_Key_Agreement_Key& server_kex_key() const;

      /**
       * @returns the agreed upon KEX group or std::nullopt if the KEX type does
       *          not depend on a group
       */
      const std::optional<Group_Params>& shared_group() const { return m_shared_group; }

      Server_Key_Exchange(Handshake_IO& io,
                          Handshake_State& state,
                          const Policy& policy,
                          Credentials_Manager& creds,
                          RandomNumberGenerator& rng,
                          const Private_Key* signing_key = nullptr);

      Server_Key_Exchange(const std::vector<uint8_t>& buf,
                          Kex_Algo kex_alg,
                          Auth_Method sig_alg,
                          Protocol_Version version);

   private:
      std::vector<uint8_t> serialize() const override;

      std::unique_ptr<PK_Key_Agreement_Key> m_kex_key;
      std::optional<Group_Params> m_shared_group;

      std::vector<uint8_t> m_params;

      std::vector<uint8_t> m_signature;
      Signature_Scheme m_scheme;
};

/**
* Server Hello Done Message
*/
class BOTAN_UNSTABLE_API Server_Hello_Done final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::ServerHelloDone; }

      explicit Server_Hello_Done(Handshake_IO& io, Handshake_Hash& hash);
      explicit Server_Hello_Done(const std::vector<uint8_t>& buf);

   private:
      std::vector<uint8_t> serialize() const override;
};

/**
* New Session Ticket Message
*/
class BOTAN_UNSTABLE_API New_Session_Ticket_12 final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::NewSessionTicket; }

      std::chrono::seconds ticket_lifetime_hint() const { return m_ticket_lifetime_hint; }

      const Session_Ticket& ticket() const { return m_ticket; }

      New_Session_Ticket_12(Handshake_IO& io,
                            Handshake_Hash& hash,
                            Session_Ticket ticket,
                            std::chrono::seconds lifetime);

      New_Session_Ticket_12(Handshake_IO& io, Handshake_Hash& hash);

      explicit New_Session_Ticket_12(const std::vector<uint8_t>& buf);

      std::vector<uint8_t> serialize() const override;

   private:
      std::chrono::seconds m_ticket_lifetime_hint;
      Session_Ticket m_ticket;
};

#if defined(BOTAN_HAS_TLS_13)

/// @brief Used to derive the ticket's PSK from the resumption_master_secret
using Ticket_Nonce = Strong<std::vector<uint8_t>, struct Ticket_Nonce_>;

class BOTAN_UNSTABLE_API New_Session_Ticket_13 final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::NewSessionTicket; }

      New_Session_Ticket_13(Ticket_Nonce nonce,
                            const Session& session,
                            const Session_Handle& handle,
                            Callbacks& callbacks);

      New_Session_Ticket_13(const std::vector<uint8_t>& buf, Connection_Side from);

      std::vector<uint8_t> serialize() const override;

      const Extensions& extensions() const { return m_extensions; }

      const Opaque_Session_Handle& handle() const { return m_handle; }

      const Ticket_Nonce& nonce() const { return m_ticket_nonce; }

      uint32_t ticket_age_add() const { return m_ticket_age_add; }

      std::chrono::seconds lifetime_hint() const { return m_ticket_lifetime_hint; }

      /**
       * @return  the number of bytes allowed for early data or std::nullopt
       *          when early data is not allowed at all
       */
      std::optional<uint32_t> early_data_byte_limit() const;

   private:
      // RFC 8446 4.6.1
      //    Clients MUST NOT cache tickets for longer than 7 days, regardless of
      //    the ticket_lifetime, and MAY delete tickets earlier based on local
      //    policy.  A server MAY treat a ticket as valid for a shorter period
      //    of time than what is stated in the ticket_lifetime.
      //
      // ... hence we call it 'lifetime hint'.
      std::chrono::seconds m_ticket_lifetime_hint;
      uint32_t m_ticket_age_add;
      Ticket_Nonce m_ticket_nonce;
      Opaque_Session_Handle m_handle;
      Extensions m_extensions;
};

#endif

/**
* Change Cipher Spec
*/
class BOTAN_UNSTABLE_API Change_Cipher_Spec final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::HandshakeCCS; }

      std::vector<uint8_t> serialize() const override { return std::vector<uint8_t>(1, 1); }
};

#if defined(BOTAN_HAS_TLS_13)

class BOTAN_UNSTABLE_API Key_Update final : public Handshake_Message {
   public:
      Handshake_Type type() const override { return Handshake_Type::KeyUpdate; }

      explicit Key_Update(bool request_peer_update);
      explicit Key_Update(const std::vector<uint8_t>& buf);

      std::vector<uint8_t> serialize() const override;

      bool expects_reciprocation() const { return m_update_requested; }

   private:
      bool m_update_requested;
};

namespace {
template <typename T>
struct as_wrapped_references {};

template <typename... AlternativeTs>
struct as_wrapped_references<std::variant<AlternativeTs...>> {
      using type = std::variant<std::reference_wrapper<AlternativeTs>...>;
};

template <typename T>
using as_wrapped_references_t = typename as_wrapped_references<T>::type;
}  // namespace

// Handshake message types from RFC 8446 4.
using Handshake_Message_13 = std::variant<Client_Hello_13,
                                          Client_Hello_12,
                                          Server_Hello_13,
                                          Server_Hello_12,
                                          Hello_Retry_Request,
                                          // End_Of_Early_Data,
                                          Encrypted_Extensions,
                                          Certificate_13,
                                          Certificate_Request_13,
                                          Certificate_Verify_13,
                                          Finished_13>;
using Handshake_Message_13_Ref = as_wrapped_references_t<Handshake_Message_13>;

using Post_Handshake_Message_13 = std::variant<New_Session_Ticket_13, Key_Update>;

// Key_Update is handled generically by the Channel. The messages assigned
// to those variants are the ones that need to be handled by the specific
// client and/or server implementations.
using Server_Post_Handshake_13_Message = std::variant<New_Session_Ticket_13, Key_Update>;
using Client_Post_Handshake_13_Message = std::variant<Key_Update>;

using Server_Handshake_13_Message = std::variant<Server_Hello_13,
                                                 Server_Hello_12,  // indicates a TLS version downgrade
                                                 Hello_Retry_Request,
                                                 Encrypted_Extensions,
                                                 Certificate_13,
                                                 Certificate_Request_13,
                                                 Certificate_Verify_13,
                                                 Finished_13>;
using Server_Handshake_13_Message_Ref = as_wrapped_references_t<Server_Handshake_13_Message>;

using Client_Handshake_13_Message = std::variant<Client_Hello_13,
                                                 Client_Hello_12,  // indicates a TLS peer that does not offer TLS 1.3
                                                 Certificate_13,
                                                 Certificate_Verify_13,
                                                 Finished_13>;
using Client_Handshake_13_Message_Ref = as_wrapped_references_t<Client_Handshake_13_Message>;

#endif  // BOTAN_HAS_TLS_13

}  // namespace TLS

}  // namespace Botan

#endif
