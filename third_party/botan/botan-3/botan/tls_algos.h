/*
* (C) 2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_ALGO_IDS_H_
#define BOTAN_TLS_ALGO_IDS_H_

#include <botan/asn1_obj.h>
#include <botan/pk_keys.h>
#include <botan/types.h>
#include <optional>
#include <string>
#include <vector>

//BOTAN_FUTURE_INTERNAL_HEADER(tls_algos.h)

namespace Botan::TLS {

enum class Cipher_Algo {
   CHACHA20_POLY1305,

   AES_128_GCM,
   AES_256_GCM,

   AES_256_OCB,

   CAMELLIA_128_GCM,
   CAMELLIA_256_GCM,

   ARIA_128_GCM,
   ARIA_256_GCM,

   AES_128_CCM,
   AES_256_CCM,
   AES_128_CCM_8,
   AES_256_CCM_8,

   AES_128_CBC_HMAC_SHA1,
   AES_128_CBC_HMAC_SHA256,
   AES_256_CBC_HMAC_SHA1,
   AES_256_CBC_HMAC_SHA256,
   AES_256_CBC_HMAC_SHA384,

   DES_EDE_CBC_HMAC_SHA1,
};

enum class KDF_Algo {
   SHA_1,
   SHA_256,
   SHA_384,
};

std::string BOTAN_DLL kdf_algo_to_string(KDF_Algo algo);

enum class Nonce_Format {
   CBC_MODE,
   AEAD_IMPLICIT_4,
   AEAD_XOR_12,
};

// TODO encoding should match signature_algorithms extension
// TODO this should include hash etc as in TLS v1.3
enum class Auth_Method {
   RSA,
   ECDSA,

   // To support TLS 1.3 ciphersuites, which do not determine the auth method
   UNDEFINED,

   // These are placed outside the encodable range
   IMPLICIT = 0x10000
};

std::string BOTAN_TEST_API auth_method_to_string(Auth_Method method);
Auth_Method BOTAN_TEST_API auth_method_from_string(std::string_view str);

/*
* Matches with wire encoding
*/
enum class Group_Params_Code : uint16_t {
   NONE = 0,

   SECP256R1 = 23,
   SECP384R1 = 24,
   SECP521R1 = 25,
   BRAINPOOL256R1 = 26,
   BRAINPOOL384R1 = 27,
   BRAINPOOL512R1 = 28,

   X25519 = 29,
   X448 = 30,

   FFDHE_2048 = 256,
   FFDHE_3072 = 257,
   FFDHE_4096 = 258,
   FFDHE_6144 = 259,
   FFDHE_8192 = 260,

   // libOQS defines those in:
   // https://github.com/open-quantum-safe/oqs-provider/blob/main/ALGORITHMS.md
   KYBER_512_R3_OQS = 0x023A,
   KYBER_768_R3_OQS = 0x023C,
   KYBER_1024_R3_OQS = 0x023D,

   eFRODOKEM_640_SHAKE_OQS = 0x0201,
   eFRODOKEM_976_SHAKE_OQS = 0x0203,
   eFRODOKEM_1344_SHAKE_OQS = 0x0205,
   eFRODOKEM_640_AES_OQS = 0x0200,
   eFRODOKEM_976_AES_OQS = 0x0202,
   eFRODOKEM_1344_AES_OQS = 0x0204,

   // Cloudflare code points for hybrid PQC
   // https://blog.cloudflare.com/post-quantum-for-all/
   HYBRID_X25519_KYBER_512_R3_CLOUDFLARE = 0xFE30,

   // libOQS defines those in:
   // https://github.com/open-quantum-safe/oqs-provider/blob/main/ALGORITHMS.md
   //
   // X25519/Kyber768 is also defined in:
   // https://datatracker.ietf.org/doc/draft-tls-westerbaan-xyber768d00/03/
   HYBRID_X25519_KYBER_512_R3_OQS = 0x2F39,
   HYBRID_X25519_KYBER_768_R3_OQS = 0x6399,

   HYBRID_X448_KYBER_768_R3_OQS = 0x2F90,

   HYBRID_SECP256R1_KYBER_512_R3_OQS = 0x2F3A,
   HYBRID_SECP256R1_KYBER_768_R3_OQS = 0x639A,

   HYBRID_SECP384R1_KYBER_768_R3_OQS = 0x2F3C,

   HYBRID_SECP521R1_KYBER_1024_R3_OQS = 0x2F3D,

   HYBRID_X25519_eFRODOKEM_640_SHAKE_OQS = 0x2F81,
   HYBRID_X25519_eFRODOKEM_640_AES_OQS = 0x2F80,

   HYBRID_X448_eFRODOKEM_976_SHAKE_OQS = 0x2F83,
   HYBRID_X448_eFRODOKEM_976_AES_OQS = 0x2F82,

   HYBRID_SECP256R1_eFRODOKEM_640_SHAKE_OQS = 0x2F01,
   HYBRID_SECP256R1_eFRODOKEM_640_AES_OQS = 0x2F00,

   HYBRID_SECP384R1_eFRODOKEM_976_SHAKE_OQS = 0x2F03,
   HYBRID_SECP384R1_eFRODOKEM_976_AES_OQS = 0x2F02,

   HYBRID_SECP521R1_eFRODOKEM_1344_SHAKE_OQS = 0x2F05,
   HYBRID_SECP521R1_eFRODOKEM_1344_AES_OQS = 0x2F04,
};

class BOTAN_PUBLIC_API(3, 2) Group_Params final {
   public:
      using enum Group_Params_Code;

      constexpr Group_Params() : m_code(Group_Params_Code::NONE) {}

      constexpr Group_Params(Group_Params_Code code) : m_code(code) {}

      constexpr Group_Params(uint16_t code) : m_code(static_cast<Group_Params_Code>(code)) {}

      /**
      * @returns std::nullopt if an unknown name
      */
      static std::optional<Group_Params> from_string(std::string_view group_name);

      constexpr bool operator==(Group_Params_Code code) const { return m_code == code; }

      constexpr bool operator==(Group_Params other) const { return m_code == other.m_code; }

      constexpr bool operator<(Group_Params other) const { return m_code < other.m_code; }

      constexpr Group_Params_Code code() const { return m_code; }

      constexpr uint16_t wire_code() const { return static_cast<uint16_t>(m_code); }

      constexpr bool is_x25519() const { return m_code == Group_Params_Code::X25519; }

      constexpr bool is_x448() const { return m_code == Group_Params_Code::X448; }

      constexpr bool is_ecdh_named_curve() const {
         return m_code == Group_Params_Code::SECP256R1 || m_code == Group_Params_Code::SECP384R1 ||
                m_code == Group_Params_Code::SECP521R1 || m_code == Group_Params_Code::BRAINPOOL256R1 ||
                m_code == Group_Params_Code::BRAINPOOL384R1 || m_code == Group_Params_Code::BRAINPOOL512R1;
      }

      constexpr bool is_in_ffdhe_range() const {
         // See RFC 7919
         return wire_code() >= 256 && wire_code() < 512;
      }

      constexpr bool is_dh_named_group() const {
         return m_code == Group_Params_Code::FFDHE_2048 || m_code == Group_Params_Code::FFDHE_3072 ||
                m_code == Group_Params_Code::FFDHE_4096 || m_code == Group_Params_Code::FFDHE_6144 ||
                m_code == Group_Params_Code::FFDHE_8192;
      }

      constexpr bool is_pure_kyber() const {
         return m_code == Group_Params_Code::KYBER_512_R3_OQS || m_code == Group_Params_Code::KYBER_768_R3_OQS ||
                m_code == Group_Params_Code::KYBER_1024_R3_OQS;
      }

      constexpr bool is_pure_frodokem() const {
         return m_code == Group_Params_Code::eFRODOKEM_640_SHAKE_OQS ||
                m_code == Group_Params_Code::eFRODOKEM_976_SHAKE_OQS ||
                m_code == Group_Params_Code::eFRODOKEM_1344_SHAKE_OQS ||
                m_code == Group_Params_Code::eFRODOKEM_640_AES_OQS ||
                m_code == Group_Params_Code::eFRODOKEM_976_AES_OQS ||
                m_code == Group_Params_Code::eFRODOKEM_1344_AES_OQS;
      }

      constexpr bool is_pure_ecc_group() const { return is_x25519() || is_x448() || is_ecdh_named_curve(); }

      constexpr bool is_post_quantum() const { return is_pure_kyber() || is_pure_frodokem() || is_pqc_hybrid(); }

      constexpr bool is_pqc_hybrid() const {
         return m_code == Group_Params_Code::HYBRID_X25519_KYBER_512_R3_CLOUDFLARE ||
                m_code == Group_Params_Code::HYBRID_X25519_KYBER_512_R3_OQS ||
                m_code == Group_Params_Code::HYBRID_X25519_KYBER_768_R3_OQS ||
                m_code == Group_Params_Code::HYBRID_X448_KYBER_768_R3_OQS ||
                m_code == Group_Params_Code::HYBRID_X25519_eFRODOKEM_640_SHAKE_OQS ||
                m_code == Group_Params_Code::HYBRID_X25519_eFRODOKEM_640_AES_OQS ||
                m_code == Group_Params_Code::HYBRID_X448_eFRODOKEM_976_SHAKE_OQS ||
                m_code == Group_Params_Code::HYBRID_X448_eFRODOKEM_976_AES_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP256R1_KYBER_512_R3_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP256R1_KYBER_768_R3_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP256R1_eFRODOKEM_640_SHAKE_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP256R1_eFRODOKEM_640_AES_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP384R1_KYBER_768_R3_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP384R1_eFRODOKEM_976_SHAKE_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP384R1_eFRODOKEM_976_AES_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP521R1_KYBER_1024_R3_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP521R1_eFRODOKEM_1344_SHAKE_OQS ||
                m_code == Group_Params_Code::HYBRID_SECP521R1_eFRODOKEM_1344_AES_OQS;
      }

      constexpr bool is_kem() const { return is_pure_kyber() || is_pure_frodokem() || is_pqc_hybrid(); }

      // Returns std::nullopt if the param has no known name
      std::optional<std::string> to_string() const;

   private:
      Group_Params_Code m_code;
};

enum class Kex_Algo {
   STATIC_RSA,
   DH,
   ECDH,
   PSK,
   ECDHE_PSK,
   DHE_PSK,
   KEM,
   KEM_PSK,
   HYBRID,
   HYBRID_PSK,

   // To support TLS 1.3 ciphersuites, which do not determine the kex algo
   UNDEFINED
};

std::string BOTAN_TEST_API kex_method_to_string(Kex_Algo method);
Kex_Algo BOTAN_TEST_API kex_method_from_string(std::string_view str);

inline bool key_exchange_is_psk(Kex_Algo m) {
   return (m == Kex_Algo::PSK || m == Kex_Algo::ECDHE_PSK || m == Kex_Algo::DHE_PSK);
}

}  // namespace Botan::TLS

#endif
