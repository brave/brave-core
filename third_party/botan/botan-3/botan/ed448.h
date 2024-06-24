/*
 * Ed448 Signature Algorithm (RFC 8032)
 * (C) 2024 Jack Lloyd
 *     2024 Fabian Albert - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_ED448_H_
#define BOTAN_ED448_H_

#include <botan/pk_keys.h>

#include <array>

namespace Botan {

/**
 * @brief A public key for Ed448/Ed448ph according to RFC 8032.
 *
 * By default, Ed448 without prehash is used (recommended). To use
 * Ed448ph, "Ed448ph" or a custom hash function identifier is passed
 * as a parameter to the create_verification_op method.
 *
 * Note that contexts (i.e. Ed448ctx) are not supported by this interface.
 */
class BOTAN_PUBLIC_API(3, 4) Ed448_PublicKey : public virtual Public_Key {
   public:
      std::string algo_name() const override { return "Ed448"; }

      size_t estimated_strength() const override { return 224; }

      size_t key_length() const override { return 448; }

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;

      std::vector<uint8_t> public_key_bits() const override;

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Signature); }

      /**
      * Create a Ed448 Public Key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      Ed448_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Create a Ed448 Public Key from bytes (57 Bytes).
      */
      Ed448_PublicKey(std::span<const uint8_t> key_bits);

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

   protected:
      Ed448_PublicKey() = default;
      std::array<uint8_t, 57> m_public;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

/**
 * @brief A private key for Ed448/Ed448ph according to RFC 8032.
 *
 * By default, Ed448 without prehash is used (recommended). To use
 * Ed448ph, "Ed448ph" or a custom hash function identifier is passed
 * as a parameter to the create_verification_op method.
 *
 * Note that contexts (i.e. Ed448ctx) are not supported by this interface.
 */
class BOTAN_PUBLIC_API(3, 4) Ed448_PrivateKey final : public Ed448_PublicKey,
                                                      public virtual Private_Key {
   public:
      /**
      * Construct a private key from the specified parameters.
      *
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      Ed448_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Construct a private key from bytes.
      *
      * @param key_bits private key bytes (57 Bytes)
      */
      Ed448_PrivateKey(std::span<const uint8_t> key_bits);

      /**
      * Generate a new private key.
      *
      * @param rng the RNG to use
      */
      explicit Ed448_PrivateKey(RandomNumberGenerator& rng);

      secure_vector<uint8_t> raw_private_key_bits() const override { return {m_private.begin(), m_private.end()}; }

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

}  // namespace Botan

#endif
