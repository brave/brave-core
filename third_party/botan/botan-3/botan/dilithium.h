/*
* Crystals Dilithium Digital Signature Algorithms
* Based on the public domain reference implementation by the
* designers (https://github.com/pq-crystals/dilithium)
*
* Further changes
* (C) 2021-2023 Jack Lloyd
* (C) 2021-2022 Manuel Glaser - Rohde & Schwarz Cybersecurity
* (C) 2021-2023 Michael Boric, René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DILITHIUM_COMMON_H_
#define BOTAN_DILITHIUM_COMMON_H_

#include <botan/pk_keys.h>

namespace Botan {

class BOTAN_PUBLIC_API(3, 0) DilithiumMode {
   public:
      enum Mode { Dilithium4x4 = 1, Dilithium4x4_AES, Dilithium6x5, Dilithium6x5_AES, Dilithium8x7, Dilithium8x7_AES };

   public:
      DilithiumMode(Mode mode) : m_mode(mode) {}

      explicit DilithiumMode(const OID& oid);
      explicit DilithiumMode(std::string_view str);

      OID object_identifier() const;
      std::string to_string() const;

      bool is_aes() const {
         return m_mode == Dilithium4x4_AES || m_mode == Dilithium6x5_AES || m_mode == Dilithium8x7_AES;
      }

      bool is_modern() const { return !is_aes(); }

      Mode mode() const { return m_mode; }

   private:
      Mode m_mode;
};

class Dilithium_PublicKeyInternal;
class Dilithium_PrivateKeyInternal;

/**
 * This implementation is based on
 * https://github.com/pq-crystals/dilithium/commit/3e9b9f1412f6c7435dbeb4e10692ea58f181ee51
 *
 * Note that this is _not_ compatible with the round 3 submission of the NIST competition.
 */
class BOTAN_PUBLIC_API(3, 0) Dilithium_PublicKey : public virtual Public_Key {
   public:
      Dilithium_PublicKey& operator=(const Dilithium_PublicKey& other) = default;

      ~Dilithium_PublicKey() override = default;

      std::string algo_name() const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      OID object_identifier() const override;

      size_t key_length() const override;

      size_t estimated_strength() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;

      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator&, bool) const override;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Signature); }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      Dilithium_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> pk);

      Dilithium_PublicKey(std::span<const uint8_t> pk, DilithiumMode mode);

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

   protected:
      Dilithium_PublicKey() = default;

      friend class Dilithium_Verification_Operation;
      friend class Dilithium_Signature_Operation;

      std::shared_ptr<Dilithium_PublicKeyInternal> m_public;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(3, 0) Dilithium_PrivateKey final : public virtual Dilithium_PublicKey,
                                                          public virtual Botan::Private_Key {
   public:
      std::unique_ptr<Public_Key> public_key() const override;

      /**
       * Generates a new key pair
       */
      Dilithium_PrivateKey(RandomNumberGenerator& rng, DilithiumMode mode);

      /**
       * Read an encoded private key.
       */
      Dilithium_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> sk);

      /**
       * Read an encoded private key given the dilithium @p mode.
       */
      Dilithium_PrivateKey(std::span<const uint8_t> sk, DilithiumMode mode);

      secure_vector<uint8_t> private_key_bits() const override;

      secure_vector<uint8_t> raw_private_key_bits() const override;

      /**
       * Create a signature operation that produces a Dilithium signature either
       * with "Randomized" or "Deterministic" rhoprime. Pass either of those
       * strings as @p params. Default (i.e. empty @p params is "Randomized").
       */
      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator&,
                                                             std::string_view params,
                                                             std::string_view provider) const override;

   private:
      friend class Dilithium_Signature_Operation;

      std::shared_ptr<Dilithium_PrivateKeyInternal> m_private;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
