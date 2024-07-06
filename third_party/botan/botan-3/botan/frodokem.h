/*
 * FrodoKEM implementation
 * Based on the MIT licensed reference implementation by the designers
 * (https://github.com/microsoft/PQCrypto-LWEKE/tree/master/src)
 *
 * The Fellowship of the FrodoKEM:
 * (C) 2023 Jack Lloyd
 *     2023 René Meusel, Amos Treiber - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_FRODOKEM_H_
#define BOTAN_FRODOKEM_H_

#include <botan/frodo_mode.h>
#include <botan/pk_keys.h>

#include <tuple>
#include <vector>

namespace Botan {

class FrodoKEM_PublicKeyInternal;
class FrodoKEM_PrivateKeyInternal;

/**
 * FrodoKEM is an unstructured lattice-based post-quantum secure KEM. It is a
 * round 3 candidate in NIST's PQC competition but was eventually not considered
 * for standardization by NIST. Nevertheless, it is endorsed by the German
 * Federal Office for Information Security for its conservative security
 * assumptions and is being standardized as an ISO standard.
 */
class BOTAN_PUBLIC_API(3, 3) FrodoKEM_PublicKey : public virtual Public_Key {
   public:
      FrodoKEM_PublicKey(std::span<const uint8_t> pub_key, FrodoKEMMode mode);

      FrodoKEM_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      FrodoKEM_PublicKey(const FrodoKEM_PublicKey& other);
      FrodoKEM_PublicKey& operator=(const FrodoKEM_PublicKey& other);
      FrodoKEM_PublicKey(FrodoKEM_PublicKey&&) = default;
      FrodoKEM_PublicKey& operator=(FrodoKEM_PublicKey&&) = default;

      ~FrodoKEM_PublicKey() override = default;

      std::string algo_name() const override { return "FrodoKEM"; }

      AlgorithmIdentifier algorithm_identifier() const override;

      OID object_identifier() const override;

      size_t key_length() const override;

      size_t estimated_strength() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;

      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator&, bool) const override;

      bool supports_operation(PublicKeyOperation op) const override {
         return (op == PublicKeyOperation::KeyEncapsulation);
      }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      std::unique_ptr<PK_Ops::KEM_Encryption> create_kem_encryption_op(std::string_view params,
                                                                       std::string_view provider) const override;

   protected:
      FrodoKEM_PublicKey() = default;

   protected:
      std::shared_ptr<FrodoKEM_PublicKeyInternal> m_public;  // NOLINT(misc-non-private-member-variables-in-classes)
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(3, 3) FrodoKEM_PrivateKey final : public virtual FrodoKEM_PublicKey,
                                                         public virtual Private_Key {
   public:
      FrodoKEM_PrivateKey(RandomNumberGenerator& rng, FrodoKEMMode mode);

      FrodoKEM_PrivateKey(std::span<const uint8_t> sk, FrodoKEMMode mode);

      FrodoKEM_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      std::unique_ptr<Public_Key> public_key() const override;

      secure_vector<uint8_t> private_key_bits() const override;

      secure_vector<uint8_t> raw_private_key_bits() const override;

      std::unique_ptr<PK_Ops::KEM_Decryption> create_kem_decryption_op(RandomNumberGenerator& rng,
                                                                       std::string_view params,
                                                                       std::string_view provider) const override;

   private:
      std::shared_ptr<FrodoKEM_PrivateKeyInternal> m_private;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
