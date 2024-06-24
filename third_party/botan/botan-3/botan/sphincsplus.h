/*
 * SPHINCS+ Hash based signature scheme
 * Based on the creative commons (CC0 1.0) reference implementation by the
 * designers (https://github.com/sphincs/sphincsplus/)
 *
 * (C) 2023 Jack Lloyd
 *     2023 Fabian Albert, René Meusel, Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_SPHINCS_PLUS_H_
#define BOTAN_SPHINCS_PLUS_H_

#include <botan/pk_keys.h>
#include <botan/sp_parameters.h>

#include <memory>
#include <vector>

namespace Botan {

class SphincsPlus_PublicKeyInternal;
class SphincsPlus_PrivateKeyInternal;

/**
 * This implementation is based on
 * https://github.com/sphincs/sphincsplus/commit/06f42f47491085ac879a72b486ca8edb10891963
 *
 * which implements SPHINCS+ Specification Round 3.1 (https://sphincs.org/data/sphincs+-r3.1-specification.pdf).
 * The used tweaked hashes are implemented according to the variant 'simple' ('robust' is not supported).
 */
class BOTAN_PUBLIC_API(3, 1) SphincsPlus_PublicKey : public virtual Public_Key {
   public:
      SphincsPlus_PublicKey(std::span<const uint8_t> pub_key, Sphincs_Parameter_Set type, Sphincs_Hash_Type hash);
      SphincsPlus_PublicKey(std::span<const uint8_t> pub_key, Sphincs_Parameters params);
      SphincsPlus_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      ~SphincsPlus_PublicKey() override;

      size_t key_length() const override;

      std::string algo_name() const override { return "SPHINCS+"; }

      size_t estimated_strength() const override;
      AlgorithmIdentifier algorithm_identifier() const override;
      OID object_identifier() const override;
      bool check_key(RandomNumberGenerator& rng, bool strong) const override;
      std::vector<uint8_t> raw_public_key_bits() const override;
      std::vector<uint8_t> public_key_bits() const override;

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

      bool supports_operation(PublicKeyOperation op) const override;

   protected:
      SphincsPlus_PublicKey() = default;

      std::shared_ptr<SphincsPlus_PublicKeyInternal> m_public;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(3, 1) SphincsPlus_PrivateKey final : public virtual SphincsPlus_PublicKey,
                                                            public virtual Private_Key {
   public:
      SphincsPlus_PrivateKey(std::span<const uint8_t> private_key, Sphincs_Parameter_Set type, Sphincs_Hash_Type hash);
      SphincsPlus_PrivateKey(std::span<const uint8_t> private_key, Sphincs_Parameters params);
      SphincsPlus_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);
      SphincsPlus_PrivateKey(RandomNumberGenerator& rng, Sphincs_Parameter_Set type, Sphincs_Hash_Type hash);
      SphincsPlus_PrivateKey(RandomNumberGenerator& rng, Sphincs_Parameters params);

      ~SphincsPlus_PrivateKey() override;

      secure_vector<uint8_t> private_key_bits() const override;
      secure_vector<uint8_t> raw_private_key_bits() const override;
      std::unique_ptr<Public_Key> public_key() const override;

      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator& rng,
                                                             std::string_view params,
                                                             std::string_view provider) const override;

   private:
      std::shared_ptr<SphincsPlus_PrivateKeyInternal> m_private;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
