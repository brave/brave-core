/*
* TLS Signature Scheme
* (C) 2022 Jack Lloyd
*     2022 Hannes Rantzsch, René Meusel - neXenio GmbH
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SIGNATURE_SCHEME_H_
#define BOTAN_TLS_SIGNATURE_SCHEME_H_

#include <botan/asn1_obj.h>
#include <botan/pk_keys.h>
#include <botan/types.h>

#include <optional>
#include <string>

namespace Botan::TLS {

class Protocol_Version;

class BOTAN_PUBLIC_API(3, 0) Signature_Scheme {
   public:
      /**
      * Matches with wire encoding
      *
      * Note that this is intentionally left as a bare enum. It emulates the Botan 2
      * API where `Signature_Scheme` was an enum class with associated free-standing
      * functions. Leaving it as a bare enum resembles the legacy user-facing API.
      */
      enum Code : uint16_t {
         NONE = 0x0000,

         RSA_PKCS1_SHA1 = 0x0201,  // not implemented
         RSA_PKCS1_SHA256 = 0x0401,
         RSA_PKCS1_SHA384 = 0x0501,
         RSA_PKCS1_SHA512 = 0x0601,

         ECDSA_SHA1 = 0x0203,  // not implemented
         ECDSA_SHA256 = 0x0403,
         ECDSA_SHA384 = 0x0503,
         ECDSA_SHA512 = 0x0603,

         RSA_PSS_SHA256 = 0x0804,
         RSA_PSS_SHA384 = 0x0805,
         RSA_PSS_SHA512 = 0x0806,

         EDDSA_25519 = 0x0807,
         EDDSA_448 = 0x0808,
      };

   public:
      /**
      * @return all available signature schemes
      */
      static const std::vector<Signature_Scheme>& all_available_schemes();

      /**
      * Construct an uninitialized / invalid scheme
      */
      Signature_Scheme();

      Signature_Scheme(uint16_t wire_code);

      Signature_Scheme(Signature_Scheme::Code wire_code);

      Signature_Scheme::Code wire_code() const noexcept { return m_code; }

      /**
      * @return true if support for this scheme is implemented in this Botan build
      */
      bool is_available() const noexcept;

      /**
      * @return true if the wire_code is set to any value other than `NONE`
      */
      bool is_set() const noexcept;

      std::string to_string() const noexcept;
      std::string hash_function_name() const noexcept;
      std::string padding_string() const noexcept;
      std::string algorithm_name() const noexcept;
      AlgorithmIdentifier key_algorithm_identifier() const noexcept;
      AlgorithmIdentifier algorithm_identifier() const noexcept;
      std::optional<Signature_Format> format() const noexcept;

      bool is_compatible_with(const Protocol_Version& protocol_version) const noexcept;
      bool is_suitable_for(const Private_Key& private_key) const noexcept;

      bool operator==(const Signature_Scheme& rhs) const { return m_code == rhs.m_code; }

      bool operator!=(const Signature_Scheme& rhs) const { return !(*this == rhs); }

   private:
      Signature_Scheme::Code m_code;
};

std::vector<AlgorithmIdentifier> to_algorithm_identifiers(const std::vector<Signature_Scheme>& schemes);

}  // namespace Botan::TLS

#endif  // BOTAN_TLS_SIGNATURE_SCHEME_H_
