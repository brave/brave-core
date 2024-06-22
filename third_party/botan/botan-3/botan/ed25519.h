/*
* Ed25519
* (C) 2017 Ribose Inc
*
* Based on the public domain code from SUPERCOP ref10 by
* Peter Schwabe, Daniel J. Bernstein, Niels Duif, Tanja Lange, Bo-Yin Yang
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ED25519_H_
#define BOTAN_ED25519_H_

#include <botan/pk_keys.h>

namespace Botan {

class BOTAN_PUBLIC_API(2, 2) Ed25519_PublicKey : public virtual Public_Key {
   public:
      std::string algo_name() const override { return "Ed25519"; }

      size_t estimated_strength() const override { return 128; }

      size_t key_length() const override { return 255; }

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;

      std::vector<uint8_t> public_key_bits() const override;

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Signature); }

      const std::vector<uint8_t>& get_public_key() const { return m_public; }

      /**
      * Create a Ed25519 Public Key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      Ed25519_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      Ed25519_PublicKey(std::span<const uint8_t> pub) : Ed25519_PublicKey(pub.data(), pub.size()) {}

      Ed25519_PublicKey(const uint8_t pub_key[], size_t len);

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

   protected:
      Ed25519_PublicKey() = default;
      std::vector<uint8_t> m_public;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 2) Ed25519_PrivateKey final : public Ed25519_PublicKey,
                                                        public virtual Private_Key {
   public:
      /**
      * Construct a private key from the specified parameters.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      Ed25519_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Generate a private key.
      * @param rng the RNG to use
      */
      explicit Ed25519_PrivateKey(RandomNumberGenerator& rng);

      /**
      * Construct a private key from the specified parameters.
      * @param secret_key the private key
      */
      explicit Ed25519_PrivateKey(const secure_vector<uint8_t>& secret_key);

      BOTAN_DEPRECATED("Use raw_private_key_bits") const secure_vector<uint8_t>& get_private_key() const {
         return m_private;
      }

      secure_vector<uint8_t> raw_private_key_bits() const override { return m_private; }

      secure_vector<uint8_t> private_key_bits() const override;

      std::unique_ptr<Public_Key> public_key() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator& rng,
                                                             std::string_view params,
                                                             std::string_view provider) const override;

   private:
      secure_vector<uint8_t> m_private;
};

BOTAN_DIAGNOSTIC_POP

void ed25519_gen_keypair(uint8_t pk[32], uint8_t sk[64], const uint8_t seed[32]);

void ed25519_sign(uint8_t sig[64],
                  const uint8_t msg[],
                  size_t msg_len,
                  const uint8_t sk[64],
                  const uint8_t domain_sep[],
                  size_t domain_sep_len);

bool ed25519_verify(const uint8_t msg[],
                    size_t msg_len,
                    const uint8_t sig[64],
                    const uint8_t pk[32],
                    const uint8_t domain_sep[],
                    size_t domain_sep_len);

}  // namespace Botan

#endif
