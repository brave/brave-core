/*
* ElGamal
* (C) 1999-2007,2023 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ELGAMAL_H_
#define BOTAN_ELGAMAL_H_

#include <botan/pk_keys.h>
#include <memory>

namespace Botan {

class BigInt;
class DL_Group;
class DL_PublicKey;
class DL_PrivateKey;

/**
* ElGamal Public Key
*/
class BOTAN_PUBLIC_API(2, 0) ElGamal_PublicKey : public virtual Public_Key {
   public:
      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Encryption); }

      /**
      * Load a public key from the ASN.1 encoding
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      ElGamal_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Create a public key.
      * @param group the underlying DL group
      * @param y the public value y = g^x mod p
      */
      ElGamal_PublicKey(const DL_Group& group, const BigInt& y);

      AlgorithmIdentifier algorithm_identifier() const override;
      std::vector<uint8_t> raw_public_key_bits() const override;
      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      size_t estimated_strength() const override;
      size_t key_length() const override;

      std::string algo_name() const override { return "ElGamal"; }

      const BigInt& get_int_field(std::string_view field) const override;

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      std::unique_ptr<PK_Ops::Encryption> create_encryption_op(RandomNumberGenerator& rng,
                                                               std::string_view params,
                                                               std::string_view provider) const override;

   private:
      friend class ElGamal_PrivateKey;

      ElGamal_PublicKey() = default;

      ElGamal_PublicKey(std::shared_ptr<const DL_PublicKey> key) : m_public_key(std::move(key)) {}

      std::shared_ptr<const DL_PublicKey> m_public_key;
};

/**
* ElGamal Private Key
*/

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) ElGamal_PrivateKey final : public ElGamal_PublicKey,
                                                        public virtual Private_Key {
   public:
      /**
      * Load a private key from the ASN.1 encoding
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded key bits in ANSI X9.42 format
      */
      ElGamal_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Create a new random private key.
      * @param rng random number generator to use
      * @param group the group to be used in the key
      */
      ElGamal_PrivateKey(RandomNumberGenerator& rng, const DL_Group& group);

      /**
      * Load a private key from the integer encoding
      * @param group the group to be used in the key
      * @param private_key the key's secret value
      */
      ElGamal_PrivateKey(const DL_Group& group, const BigInt& private_key);

      bool check_key(RandomNumberGenerator& rng, bool) const override;

      std::unique_ptr<Public_Key> public_key() const override;

      secure_vector<uint8_t> private_key_bits() const override;

      secure_vector<uint8_t> raw_private_key_bits() const override;

      const BigInt& get_int_field(std::string_view field) const override;

      std::unique_ptr<PK_Ops::Decryption> create_decryption_op(RandomNumberGenerator& rng,
                                                               std::string_view params,
                                                               std::string_view provider) const override;

   private:
      std::shared_ptr<const DL_PrivateKey> m_private_key;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
