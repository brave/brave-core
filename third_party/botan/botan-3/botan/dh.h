/*
* Diffie-Hellman
* (C) 1999-2007,2023 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DIFFIE_HELLMAN_H_
#define BOTAN_DIFFIE_HELLMAN_H_

#include <botan/pk_keys.h>
#include <memory>

namespace Botan {

class BigInt;
class DL_Group;
class DL_PublicKey;
class DL_PrivateKey;

/**
* This class represents Diffie-Hellman public keys.
*/
class BOTAN_PUBLIC_API(2, 0) DH_PublicKey : public virtual Public_Key {
   public:
      /**
      * Create a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      DH_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Construct a public key with the specified parameters.
      * @param group the DL group to use in the key
      * @param y the public value y
      */
      DH_PublicKey(const DL_Group& group, const BigInt& y);

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;

      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      size_t estimated_strength() const override;
      size_t key_length() const override;

      std::vector<uint8_t> public_value() const;

      std::string algo_name() const override { return "DH"; }

      const BigInt& get_int_field(std::string_view field) const override;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::KeyAgreement); }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      const DL_Group& group() const;

   private:
      friend class DH_PrivateKey;

      DH_PublicKey() = default;

      DH_PublicKey(std::shared_ptr<const DL_PublicKey> key) : m_public_key(std::move(key)) {}

      std::shared_ptr<const DL_PublicKey> m_public_key;
};

/**
* This class represents Diffie-Hellman private keys.
*/

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) DH_PrivateKey final : public DH_PublicKey,
                                                   public PK_Key_Agreement_Key,
                                                   public virtual Private_Key {
   public:
      /**
      * Load a private key from the ASN.1 encoding
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      DH_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Load a private key from the integer encoding
      * @param group the underlying DL group
      * @param private_key the private key
      */
      DH_PrivateKey(const DL_Group& group, const BigInt& private_key);

      /**
      * Create a new private key.
      * @param group the underlying DL group
      * @param rng the RNG to use
      */
      DH_PrivateKey(RandomNumberGenerator& rng, const DL_Group& group);

      std::unique_ptr<Public_Key> public_key() const override;

      std::vector<uint8_t> public_value() const override;

      secure_vector<uint8_t> private_key_bits() const override;

      secure_vector<uint8_t> raw_private_key_bits() const override;

      const BigInt& get_int_field(std::string_view field) const override;

      std::unique_ptr<PK_Ops::Key_Agreement> create_key_agreement_op(RandomNumberGenerator& rng,
                                                                     std::string_view params,
                                                                     std::string_view provider) const override;

   private:
      std::shared_ptr<const DL_PrivateKey> m_private_key;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
