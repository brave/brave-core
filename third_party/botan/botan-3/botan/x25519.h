/*
* (C) 2014,2024 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X25519_H_
#define BOTAN_X25519_H_

#include <botan/pk_keys.h>

namespace Botan {

class BOTAN_PUBLIC_API(2, 0) X25519_PublicKey : public virtual Public_Key {
   public:
      std::string algo_name() const override { return "X25519"; }

      size_t estimated_strength() const override { return 128; }

      size_t key_length() const override { return 255; }

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;

      std::vector<uint8_t> public_key_bits() const override;

      std::vector<uint8_t> public_value() const { return m_public; }

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::KeyAgreement); }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      /**
      * Create a X25519 Public Key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      X25519_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Create a X25519 Public Key.
      * @param pub 32-byte raw public key
      */
      explicit X25519_PublicKey(std::span<const uint8_t> pub);

   protected:
      X25519_PublicKey() = default;
      std::vector<uint8_t> m_public;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) X25519_PrivateKey final : public X25519_PublicKey,
                                                       public virtual Private_Key,
                                                       public virtual PK_Key_Agreement_Key {
   public:
      /**
      * Construct a private key from the specified parameters.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      X25519_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Generate a private key.
      * @param rng the RNG to use
      */
      explicit X25519_PrivateKey(RandomNumberGenerator& rng);

      /**
      * Construct a private key from the specified parameters.
      * @param secret_key the private key
      */
      explicit X25519_PrivateKey(const secure_vector<uint8_t>& secret_key);

      std::vector<uint8_t> public_value() const override { return X25519_PublicKey::public_value(); }

      secure_vector<uint8_t> agree(const uint8_t w[], size_t w_len) const;

      secure_vector<uint8_t> raw_private_key_bits() const override { return m_private; }

      BOTAN_DEPRECATED("Use raw_private_key_bits") const secure_vector<uint8_t>& get_x() const { return m_private; }

      secure_vector<uint8_t> private_key_bits() const override;

      std::unique_ptr<Public_Key> public_key() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      std::unique_ptr<PK_Ops::Key_Agreement> create_key_agreement_op(RandomNumberGenerator& rng,
                                                                     std::string_view params,
                                                                     std::string_view provider) const override;

   private:
      secure_vector<uint8_t> m_private;
};

BOTAN_DIAGNOSTIC_POP

/*
* The types above are just wrappers for curve25519_donna, plus defining
* encodings for public and private keys.
*/
BOTAN_DEPRECATED_API("Use X25519_PrivateKey or Sodium::crypto_scalarmult_curve25519")
void curve25519_donna(uint8_t mypublic[32], const uint8_t secret[32], const uint8_t basepoint[32]);

/**
* Exponentiate by the x25519 base point
* @param mypublic output value
* @param secret random scalar
*/
BOTAN_DEPRECATED_API("Use X25519_PrivateKey or Sodium::crypto_scalarmult_curve25519_base")
void curve25519_basepoint(uint8_t mypublic[32], const uint8_t secret[32]);

}  // namespace Botan

#endif
