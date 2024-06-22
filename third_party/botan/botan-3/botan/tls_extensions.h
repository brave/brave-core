/*
* TLS Extensions
* (C) 2011,2012,2016,2018,2019 Jack Lloyd
* (C) 2016 Juraj Somorovsky
* (C) 2016 Matthias Gierlings
* (C) 2021 Elektrobit Automotive GmbH
* (C) 2022 René Meusel, Hannes Rantzsch - neXenio GmbH
* (C) 2023 Fabian Albert, René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_EXTENSIONS_H_
#define BOTAN_TLS_EXTENSIONS_H_

#include <botan/credentials_manager.h>
#include <botan/pkix_types.h>
#include <botan/secmem.h>
#include <botan/tls_algos.h>
#include <botan/tls_magic.h>
#include <botan/tls_session.h>
#include <botan/tls_signature_scheme.h>
#include <botan/tls_version.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace Botan {

class RandomNumberGenerator;
class Credentials_Manager;

namespace TLS {

#if defined(BOTAN_HAS_TLS_13)
class Callbacks;
class Session_Manager;
class Cipher_State;
class Ciphersuite;
class Transcript_Hash_State;

enum class PSK_Key_Exchange_Mode : uint8_t { PSK_KE = 0, PSK_DHE_KE = 1 };

#endif
class Policy;
class TLS_Data_Reader;

enum class Extension_Code : uint16_t {
   ServerNameIndication = 0,
   CertificateStatusRequest = 5,

   SupportedGroups = 10,
   EcPointFormats = 11,
   SignatureAlgorithms = 13,
   CertSignatureAlgorithms = 50,
   UseSrtp = 14,
   ApplicationLayerProtocolNegotiation = 16,

   // SignedCertificateTimestamp          = 18,  // NYI

   // RFC 7250 (Raw Public Keys in TLS)
   ClientCertificateType = 19,
   ServerCertificateType = 20,

   EncryptThenMac = 22,
   ExtendedMasterSecret = 23,

   RecordSizeLimit = 28,

   SessionTicket = 35,

   SupportedVersions = 43,
#if defined(BOTAN_HAS_TLS_13)
   PresharedKey = 41,
   EarlyData = 42,
   Cookie = 44,

   PskKeyExchangeModes = 45,
   CertificateAuthorities = 47,
   // OidFilters                          = 48,  // NYI

   KeyShare = 51,
#endif

   SafeRenegotiation = 65281,
};

/**
* Base class representing a TLS extension of some kind
*/
class BOTAN_UNSTABLE_API Extension {
   public:
      /**
      * @return code number of the extension
      */
      virtual Extension_Code type() const = 0;

      /**
      * @return serialized binary for the extension
      */
      virtual std::vector<uint8_t> serialize(Connection_Side whoami) const = 0;

      /**
      * @return if we should encode this extension or not
      */
      virtual bool empty() const = 0;

      /**
       * @return true if this extension is known and implemented by Botan
       */
      virtual bool is_implemented() const { return true; }

      virtual ~Extension() = default;
};

/**
* Server Name Indicator extension (RFC 3546)
*/
class BOTAN_UNSTABLE_API Server_Name_Indicator final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::ServerNameIndication; }

      Extension_Code type() const override { return static_type(); }

      explicit Server_Name_Indicator(std::string_view host_name) : m_sni_host_name(host_name) {}

      Server_Name_Indicator(TLS_Data_Reader& reader, uint16_t extension_size);

      std::string host_name() const { return m_sni_host_name; }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return false; }

   private:
      std::string m_sni_host_name;
};

/**
* Renegotiation Indication Extension (RFC 5746)
*/
class BOTAN_UNSTABLE_API Renegotiation_Extension final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::SafeRenegotiation; }

      Extension_Code type() const override { return static_type(); }

      Renegotiation_Extension() = default;

      explicit Renegotiation_Extension(const std::vector<uint8_t>& bits) : m_reneg_data(bits) {}

      Renegotiation_Extension(TLS_Data_Reader& reader, uint16_t extension_size);

      const std::vector<uint8_t>& renegotiation_info() const { return m_reneg_data; }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return false; }  // always send this

   private:
      std::vector<uint8_t> m_reneg_data;
};

/**
* ALPN (RFC 7301)
*/
class BOTAN_UNSTABLE_API Application_Layer_Protocol_Notification final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::ApplicationLayerProtocolNegotiation; }

      Extension_Code type() const override { return static_type(); }

      const std::vector<std::string>& protocols() const { return m_protocols; }

      std::string single_protocol() const;

      /**
      * Single protocol, used by server
      */
      explicit Application_Layer_Protocol_Notification(std::string_view protocol) :
            m_protocols(1, std::string(protocol)) {}

      /**
      * List of protocols, used by client
      */
      explicit Application_Layer_Protocol_Notification(const std::vector<std::string>& protocols) :
            m_protocols(protocols) {}

      Application_Layer_Protocol_Notification(TLS_Data_Reader& reader, uint16_t extension_size, Connection_Side from);

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_protocols.empty(); }

   private:
      std::vector<std::string> m_protocols;
};

// As defined in RFC 8446 4.4.2
enum class Certificate_Type : uint8_t { X509 = 0, RawPublicKey = 2 };

std::string certificate_type_to_string(Certificate_Type type);
Certificate_Type certificate_type_from_string(const std::string& type_str);

/**
 * RFC 7250
 * Base class for 'client_certificate_type' and 'server_certificate_type' extensions.
 */
class BOTAN_UNSTABLE_API Certificate_Type_Base : public Extension {
   public:
      /**
       * Called by the client to advertise support for a number of cert types.
       */
      Certificate_Type_Base(std::vector<Certificate_Type> supported_cert_types);

   protected:
      /**
       * Called by the server to select a cert type to be used in the handshake.
       */
      Certificate_Type_Base(const Certificate_Type_Base& certificate_type_from_client,
                            const std::vector<Certificate_Type>& server_preference);

   public:
      Certificate_Type_Base(TLS_Data_Reader& reader, uint16_t extension_size, Connection_Side from);

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      void validate_selection(const Certificate_Type_Base& from_server) const;
      Certificate_Type selected_certificate_type() const;

      bool empty() const override {
         // RFC 7250 4.1
         //    If the client has no remaining certificate types to send in the
         //    client hello, other than the default X.509 type, it MUST omit the
         //    entire client[/server]_certificate_type extension [...].
         return m_from == Connection_Side::Client && m_certificate_types.size() == 1 &&
                m_certificate_types.front() == Certificate_Type::X509;
      }

   private:
      std::vector<Certificate_Type> m_certificate_types;
      Connection_Side m_from;
};

class BOTAN_UNSTABLE_API Client_Certificate_Type final : public Certificate_Type_Base {
   public:
      using Certificate_Type_Base::Certificate_Type_Base;

      /**
       * Creates the Server Hello extension from the received client preferences.
       */
      Client_Certificate_Type(const Client_Certificate_Type& cct, const Policy& policy);

      static Extension_Code static_type() { return Extension_Code::ClientCertificateType; }

      Extension_Code type() const override { return static_type(); }
};

class BOTAN_UNSTABLE_API Server_Certificate_Type final : public Certificate_Type_Base {
   public:
      using Certificate_Type_Base::Certificate_Type_Base;

      /**
       * Creates the Server Hello extension from the received client preferences.
       */
      Server_Certificate_Type(const Server_Certificate_Type& sct, const Policy& policy);

      static Extension_Code static_type() { return Extension_Code::ServerCertificateType; }

      Extension_Code type() const override { return static_type(); }
};

/**
* Session Ticket Extension (RFC 5077)
*/
class BOTAN_UNSTABLE_API Session_Ticket_Extension final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::SessionTicket; }

      Extension_Code type() const override { return static_type(); }

      /**
      * @return contents of the session ticket
      */
      const Session_Ticket& contents() const { return m_ticket; }

      /**
      * Create empty extension, used by both client and server
      */
      Session_Ticket_Extension() = default;

      /**
      * Extension with ticket, used by client
      */
      explicit Session_Ticket_Extension(Session_Ticket session_ticket) : m_ticket(std::move(session_ticket)) {}

      /**
      * Deserialize a session ticket
      */
      Session_Ticket_Extension(TLS_Data_Reader& reader, uint16_t extension_size);

      std::vector<uint8_t> serialize(Connection_Side) const override { return m_ticket.get(); }

      bool empty() const override { return false; }

   private:
      Session_Ticket m_ticket;
};

/**
* Supported Groups Extension (RFC 7919)
*/
class BOTAN_UNSTABLE_API Supported_Groups final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::SupportedGroups; }

      Extension_Code type() const override { return static_type(); }

      const std::vector<Group_Params>& groups() const;

      // Returns the list of groups we recognize as ECDH curves
      std::vector<Group_Params> ec_groups() const;

      // Returns the list of any groups in the FFDHE range
      std::vector<Group_Params> dh_groups() const;

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      explicit Supported_Groups(const std::vector<Group_Params>& groups);

      Supported_Groups(TLS_Data_Reader& reader, uint16_t extension_size);

      bool empty() const override { return m_groups.empty(); }

   private:
      std::vector<Group_Params> m_groups;
};

// previously Supported Elliptic Curves Extension (RFC 4492)
//using Supported_Elliptic_Curves = Supported_Groups;

/**
* Supported Point Formats Extension (RFC 4492)
*/
class BOTAN_UNSTABLE_API Supported_Point_Formats final : public Extension {
   public:
      enum ECPointFormat : uint8_t {
         UNCOMPRESSED = 0,
         ANSIX962_COMPRESSED_PRIME = 1,
         ANSIX962_COMPRESSED_CHAR2 = 2,  // don't support these curves
      };

      static Extension_Code static_type() { return Extension_Code::EcPointFormats; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      explicit Supported_Point_Formats(bool prefer_compressed) : m_prefers_compressed(prefer_compressed) {}

      Supported_Point_Formats(TLS_Data_Reader& reader, uint16_t extension_size);

      bool empty() const override { return false; }

      bool prefers_compressed() const { return m_prefers_compressed; }

   private:
      bool m_prefers_compressed = false;
};

/**
* Signature Algorithms Extension for TLS 1.2 (RFC 5246)
*/
class BOTAN_UNSTABLE_API Signature_Algorithms final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::SignatureAlgorithms; }

      Extension_Code type() const override { return static_type(); }

      const std::vector<Signature_Scheme>& supported_schemes() const { return m_schemes; }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_schemes.empty(); }

      explicit Signature_Algorithms(std::vector<Signature_Scheme> schemes) : m_schemes(std::move(schemes)) {}

      Signature_Algorithms(TLS_Data_Reader& reader, uint16_t extension_size);

   private:
      std::vector<Signature_Scheme> m_schemes;
};

/**
* Signature_Algorithms_Cert for TLS 1.3 (RFC 8446)
*
* RFC 8446 4.2.3
*    TLS 1.3 provides two extensions for indicating which signature algorithms
*    may be used in digital signatures.  The "signature_algorithms_cert"
*    extension applies to signatures in certificates, and the
*    "signature_algorithms" extension, which originally appeared in TLS 1.2,
*    applies to signatures in CertificateVerify messages.
*
* RFC 8446 4.2.3
*    TLS 1.2 implementations SHOULD also process this extension.
*/
class BOTAN_UNSTABLE_API Signature_Algorithms_Cert final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::CertSignatureAlgorithms; }

      Extension_Code type() const override { return static_type(); }

      const std::vector<Signature_Scheme>& supported_schemes() const { return m_schemes; }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_schemes.empty(); }

      explicit Signature_Algorithms_Cert(std::vector<Signature_Scheme> schemes) : m_schemes(std::move(schemes)) {}

      Signature_Algorithms_Cert(TLS_Data_Reader& reader, uint16_t extension_size);

   private:
      std::vector<Signature_Scheme> m_schemes;
};

/**
* Used to indicate SRTP algorithms for DTLS (RFC 5764)
*/
class BOTAN_UNSTABLE_API SRTP_Protection_Profiles final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::UseSrtp; }

      Extension_Code type() const override { return static_type(); }

      const std::vector<uint16_t>& profiles() const { return m_pp; }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_pp.empty(); }

      explicit SRTP_Protection_Profiles(const std::vector<uint16_t>& pp) : m_pp(pp) {}

      explicit SRTP_Protection_Profiles(uint16_t pp) : m_pp(1, pp) {}

      SRTP_Protection_Profiles(TLS_Data_Reader& reader, uint16_t extension_size);

   private:
      std::vector<uint16_t> m_pp;
};

/**
* Extended Master Secret Extension (RFC 7627)
*/
class BOTAN_UNSTABLE_API Extended_Master_Secret final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::ExtendedMasterSecret; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return false; }

      Extended_Master_Secret() = default;

      Extended_Master_Secret(TLS_Data_Reader& reader, uint16_t extension_size);
};

/**
* Encrypt-then-MAC Extension (RFC 7366)
*/
class BOTAN_UNSTABLE_API Encrypt_then_MAC final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::EncryptThenMac; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return false; }

      Encrypt_then_MAC() = default;

      Encrypt_then_MAC(TLS_Data_Reader& reader, uint16_t extension_size);
};

class Certificate_Status_Request_Internal;

/**
* Certificate Status Request (RFC 6066)
*/
class BOTAN_UNSTABLE_API Certificate_Status_Request final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::CertificateStatusRequest; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return false; }

      const std::vector<uint8_t>& get_responder_id_list() const;
      const std::vector<uint8_t>& get_request_extensions() const;
      const std::vector<uint8_t>& get_ocsp_response() const;

      // TLS 1.2 Server generated version: empty
      Certificate_Status_Request();

      // TLS 1.2 Client version, both lists can be empty
      Certificate_Status_Request(std::vector<uint8_t> ocsp_responder_ids,
                                 std::vector<std::vector<uint8_t>> ocsp_key_ids);

      // TLS 1.3 version
      Certificate_Status_Request(std::vector<uint8_t> response);

      Certificate_Status_Request(TLS_Data_Reader& reader,
                                 uint16_t extension_size,
                                 Handshake_Type message_type,
                                 Connection_Side from);

      ~Certificate_Status_Request() override;

   private:
      std::unique_ptr<Certificate_Status_Request_Internal> m_impl;
};

/**
* Supported Versions from RFC 8446
*/
class BOTAN_UNSTABLE_API Supported_Versions final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::SupportedVersions; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_versions.empty(); }

      Supported_Versions(Protocol_Version version, const Policy& policy);

      Supported_Versions(Protocol_Version version) { m_versions.push_back(version); }

      Supported_Versions(TLS_Data_Reader& reader, uint16_t extension_size, Connection_Side from);

      bool supports(Protocol_Version version) const;

      const std::vector<Protocol_Version>& versions() const { return m_versions; }

   private:
      std::vector<Protocol_Version> m_versions;
};

using Named_Group = Group_Params;

/**
* Record Size Limit (RFC 8449)
*
* TODO: the record size limit is currently not honored by the TLS 1.2 stack
*/
class BOTAN_UNSTABLE_API Record_Size_Limit final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::RecordSizeLimit; }

      Extension_Code type() const override { return static_type(); }

      explicit Record_Size_Limit(uint16_t limit);

      Record_Size_Limit(TLS_Data_Reader& reader, uint16_t extension_size, Connection_Side from);

      uint16_t limit() const { return m_limit; }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_limit == 0; }

   private:
      uint16_t m_limit;
};

using Named_Group = Group_Params;

#if defined(BOTAN_HAS_TLS_13)
/**
* Cookie from RFC 8446 4.2.2
*/
class BOTAN_UNSTABLE_API Cookie final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::Cookie; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_cookie.empty(); }

      const std::vector<uint8_t>& get_cookie() const { return m_cookie; }

      explicit Cookie(const std::vector<uint8_t>& cookie);

      explicit Cookie(TLS_Data_Reader& reader, uint16_t extension_size);

   private:
      std::vector<uint8_t> m_cookie;
};

/**
* Pre-Shared Key Exchange Modes from RFC 8446 4.2.9
*/
class BOTAN_UNSTABLE_API PSK_Key_Exchange_Modes final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::PskKeyExchangeModes; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_modes.empty(); }

      const std::vector<PSK_Key_Exchange_Mode>& modes() const { return m_modes; }

      explicit PSK_Key_Exchange_Modes(std::vector<PSK_Key_Exchange_Mode> modes) : m_modes(std::move(modes)) {}

      explicit PSK_Key_Exchange_Modes(TLS_Data_Reader& reader, uint16_t extension_size);

   private:
      std::vector<PSK_Key_Exchange_Mode> m_modes;
};

/**
 * Certificate Authorities Extension from RFC 8446 4.2.4
 */
class BOTAN_UNSTABLE_API Certificate_Authorities final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::CertificateAuthorities; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override { return m_distinguished_names.empty(); }

      const std::vector<X509_DN>& distinguished_names() const { return m_distinguished_names; }

      Certificate_Authorities(TLS_Data_Reader& reader, uint16_t extension_size);
      explicit Certificate_Authorities(std::vector<X509_DN> acceptable_DNs);

   private:
      std::vector<X509_DN> m_distinguished_names;
};

/**
 * Pre-Shared Key extension from RFC 8446 4.2.11
 */
class BOTAN_UNSTABLE_API PSK final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::PresharedKey; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side side) const override;

      /**
       * Returns the PSK identity (in case of an externally provided PSK) and
       * the cipher state representing the PSK selected by the server. Note that
       * this destructs the list of offered PSKs and its cipher states and must
       * therefore not be called more than once.
       *
       * @note Technically, PSKs used for resumption also carry an identity.
       *       Though, typically, this is an opaque value meaningful only to the
       *       peer and of no authorative value for the user. We therefore
       *       report the identity of externally provided PSKs only.
       */
      std::pair<std::optional<std::string>, std::unique_ptr<Cipher_State>> take_selected_psk_info(
         const PSK& server_psk, const Ciphersuite& cipher);

      /**
       * Selects one of the offered PSKs that is compatible with \p cipher.
       * @retval PSK extension object that can be added to the Server Hello response
       * @retval std::nullptr if no PSK offered by the client is convenient
       */
      std::unique_ptr<PSK> select_offered_psk(std::string_view host,
                                              const Ciphersuite& cipher,
                                              Session_Manager& session_mgr,
                                              Credentials_Manager& credentials_mgr,
                                              Callbacks& callbacks,
                                              const Policy& policy);

      /**
       * Remove PSK identities from the list in \p m_psk that are not compatible
       * with the passed in \p cipher suite.
       * This is useful to react to Hello Retry Requests. See RFC 8446 4.1.4.
       */
      void filter(const Ciphersuite& cipher);

      /**
       * Pulls the preshared key or the Session to resume from a PSK extension
       * in Server Hello.
       */
      std::variant<Session, ExternalPSK> take_session_to_resume_or_psk();

      bool empty() const override;

      PSK(TLS_Data_Reader& reader, uint16_t extension_size, Handshake_Type message_type);

      /**
       * Creates a PSK extension with a TLS 1.3 session object containing a
       * master_secret. Note that it will extract that secret from the session,
       * and won't create a copy of it.
       *
       * @param session_to_resume  the session to be resumed; note that the
       *                           master secret will be taken away from the
       *                           session object.
       * @param psks               a list of non-resumption PSKs that should be
       *                           offered to the server
       * @param callbacks          the application's callbacks
       */
      PSK(std::optional<Session_with_Handle>& session_to_resume, std::vector<ExternalPSK> psks, Callbacks& callbacks);

      ~PSK() override;

      void calculate_binders(const Transcript_Hash_State& truncated_transcript_hash);
      bool validate_binder(const PSK& server_psk, const std::vector<uint8_t>& binder) const;

      // TODO: Implement pure PSK negotiation that is not used for session
      //       resumption.

   private:
      /**
       * Creates a PSK extension that specifies the server's selection of an
       * offered client PSK. The @p session_to_resume is kept internally
       * and used later for the initialization of the Cipher_State object.
       *
       * Note: This constructor is called internally in PSK::select_offered_psk().
       */
      PSK(Session session_to_resume, uint16_t psk_index);

      /**
       * Creates a PSK extension that specifies the server's selection of an
       * externally provided PSK offered by the client. The @p psk is kept
       * internally and used later for the initialization of the Cipher_State object.
       *
       * Note: This constructor is called internally in PSK::select_offered_psk().
       */
      PSK(ExternalPSK psk, const uint16_t psk_index);

   private:
      class PSK_Internal;
      std::unique_ptr<PSK_Internal> m_impl;
};

/**
* Key_Share from RFC 8446 4.2.8
*/
class BOTAN_UNSTABLE_API Key_Share final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::KeyShare; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override;

      /**
       * Creates a Key_Share extension meant for the Server Hello that
       * performs a key encapsulation with the selected public key from
       * the client.
       *
       * @note This will retain the shared secret in the Key_Share extension
       *       until it is retrieved via take_shared_secret().
       */
      static std::unique_ptr<Key_Share> create_as_encapsulation(Group_Params selected_group,
                                                                const Key_Share& client_keyshare,
                                                                const Policy& policy,
                                                                Callbacks& cb,
                                                                RandomNumberGenerator& rng);

      /**
       * Decapsulate the shared secret with the peer's key share. This method
       * can be called on a ClientHello's Key_Share with a ServerHello's
       * Key_Share.
       *
       * @note After the decapsulation the client's private key is destroyed.
       *       Multiple calls will result in an exception.
       */
      secure_vector<uint8_t> decapsulate(const Key_Share& server_keyshare,
                                         const Policy& policy,
                                         Callbacks& cb,
                                         RandomNumberGenerator& rng);

      /**
       * Update a ClientHello's Key_Share to comply with a HelloRetryRequest.
       *
       * This will create new Key_Share_Entries and should only be called on a ClientHello Key_Share with a HelloRetryRequest Key_Share.
       */
      void retry_offer(const Key_Share& retry_request_keyshare,
                       const std::vector<Named_Group>& supported_groups,
                       Callbacks& cb,
                       RandomNumberGenerator& rng);

      /**
       * @return key exchange groups the peer offered key share entries for
       */
      std::vector<Named_Group> offered_groups() const;

      /**
       * @return key exchange group that was selected by a Hello Retry Request
       */
      Named_Group selected_group() const;

      /**
       * @returns the shared secret that was obtained by constructing this
       *          Key_Share object with the peer's.
       *
       * @note the shared secret value is std:move'd out. Multiple calls will
       *       result in an exception.
       */
      secure_vector<uint8_t> take_shared_secret();

      Key_Share(TLS_Data_Reader& reader, uint16_t extension_size, Handshake_Type message_type);

      // constructor used for ClientHello msg
      Key_Share(const Policy& policy, Callbacks& cb, RandomNumberGenerator& rng);

      // constructor used for HelloRetryRequest msg
      explicit Key_Share(Named_Group selected_group);

      // destructor implemented in .cpp to hide Key_Share_Impl
      ~Key_Share() override;

   private:
      // constructor used for ServerHello
      // (called via create_as_encapsulation())
      Key_Share(Group_Params selected_group,
                const Key_Share& client_keyshare,
                const Policy& policy,
                Callbacks& cb,
                RandomNumberGenerator& rng);

   private:
      class Key_Share_Impl;
      std::unique_ptr<Key_Share_Impl> m_impl;
};

/**
 * Indicates usage or support of early data as described in RFC 8446 4.2.10.
 */
class BOTAN_UNSTABLE_API EarlyDataIndication final : public Extension {
   public:
      static Extension_Code static_type() { return Extension_Code::EarlyData; }

      Extension_Code type() const override { return static_type(); }

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      bool empty() const override;

      std::optional<uint32_t> max_early_data_size() const { return m_max_early_data_size; }

      EarlyDataIndication(TLS_Data_Reader& reader, uint16_t extension_size, Handshake_Type message_type);

      /**
       * The max_early_data_size is exclusively provided by servers when using
       * this extension in the NewSessionTicket message! Otherwise it stays
       * std::nullopt and results in an empty extension. (RFC 8446 4.2.10).
       */
      EarlyDataIndication(std::optional<uint32_t> max_early_data_size = std::nullopt) :
            m_max_early_data_size(std::move(max_early_data_size)) {}

   private:
      std::optional<uint32_t> m_max_early_data_size;
};

#endif

/**
* Unknown extensions are deserialized as this type
*/
class BOTAN_UNSTABLE_API Unknown_Extension final : public Extension {
   public:
      Unknown_Extension(Extension_Code type, TLS_Data_Reader& reader, uint16_t extension_size);

      std::vector<uint8_t> serialize(Connection_Side whoami) const override;

      const std::vector<uint8_t>& value() { return m_value; }

      bool empty() const override { return false; }

      Extension_Code type() const override { return m_type; }

      bool is_implemented() const override { return false; }

   private:
      Extension_Code m_type;
      std::vector<uint8_t> m_value;
};

/**
* Represents a block of extensions in a hello message
*/
class BOTAN_UNSTABLE_API Extensions final {
   public:
      std::set<Extension_Code> extension_types() const;

      const std::vector<std::unique_ptr<Extension>>& all() const { return m_extensions; }

      template <typename T>
      T* get() const {
         return dynamic_cast<T*>(get(T::static_type()));
      }

      template <typename T>
      bool has() const {
         return get<T>() != nullptr;
      }

      bool has(Extension_Code type) const { return get(type) != nullptr; }

      size_t size() const { return m_extensions.size(); }

      bool empty() const { return m_extensions.empty(); }

      void add(std::unique_ptr<Extension> extn);

      void add(Extension* extn) { add(std::unique_ptr<Extension>(extn)); }

      Extension* get(Extension_Code type) const {
         const auto i = std::find_if(
            m_extensions.cbegin(), m_extensions.cend(), [type](const auto& ext) { return ext->type() == type; });

         return (i != m_extensions.end()) ? i->get() : nullptr;
      }

      std::vector<uint8_t> serialize(Connection_Side whoami) const;

      void deserialize(TLS_Data_Reader& reader, Connection_Side from, Handshake_Type message_type);

      /**
       * @param allowed_extensions        extension types that are allowed
       * @param allow_unknown_extensions  if true, ignores unrecognized extensions
       * @returns true if this contains any extensions that are not contained in @p allowed_extensions.
       */
      bool contains_other_than(const std::set<Extension_Code>& allowed_extensions,
                               bool allow_unknown_extensions = false) const;

      /**
       * @param allowed_extensions  extension types that are allowed
       * @returns true if this contains any extensions implemented by Botan that
       *          are not contained in @p allowed_extensions.
       */
      bool contains_implemented_extensions_other_than(const std::set<Extension_Code>& allowed_extensions) const {
         return contains_other_than(allowed_extensions, true);
      }

      /**
       * Take the extension with the given type out of the extensions list.
       * Returns a nullptr if the extension didn't exist.
       */
      template <typename T>
      decltype(auto) take() {
         std::unique_ptr<T> out_ptr;

         auto ext = take(T::static_type());
         if(ext != nullptr) {
            out_ptr.reset(dynamic_cast<T*>(ext.get()));
            BOTAN_ASSERT_NOMSG(out_ptr != nullptr);
            ext.release();
         }

         return out_ptr;
      }

      /**
       * Take the extension with the given type out of the extensions list.
       * Returns a nullptr if the extension didn't exist.
       */
      std::unique_ptr<Extension> take(Extension_Code type);

      /**
      * Remove an extension from this extensions object, if it exists.
      * Returns true if the extension existed (and thus is now removed),
      * otherwise false (the extension wasn't set in the first place).
      *
      * Note: not used internally, might be used in Callbacks::tls_modify_extensions()
      */
      bool remove_extension(Extension_Code type) { return take(type) != nullptr; }

      Extensions() = default;
      Extensions(const Extensions&) = delete;
      Extensions& operator=(const Extensions&) = delete;
      Extensions(Extensions&&) = default;
      Extensions& operator=(Extensions&&) = default;

      Extensions(TLS_Data_Reader& reader, Connection_Side side, Handshake_Type message_type) {
         deserialize(reader, side, message_type);
      }

   private:
      std::vector<std::unique_ptr<Extension>> m_extensions;
};

}  // namespace TLS

}  // namespace Botan

#endif
