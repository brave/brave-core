/*
 * Crystals Kyber key encapsulation mechanism
 * Based on the public domain reference implementation by the
 * designers (https://github.com/pq-crystals/kyber)
 *
 * Further changes
 * (C) 2021-2022 Jack Lloyd
 * (C) 2021-2022 Manuel Glaser and Michael Boric, Rohde & Schwarz Cybersecurity
 * (C) 2021-2022 René Meusel and Hannes Rantzsch, neXenio GmbH
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_KYBER_COMMON_H_
#define BOTAN_KYBER_COMMON_H_

#include <botan/asn1_obj.h>
#include <botan/der_enc.h>
#include <botan/exceptn.h>
#include <botan/pk_keys.h>

#include <span>

#if !defined(BOTAN_HAS_KYBER_90S) && !defined(BOTAN_HAS_KYBER)
static_assert(false, "botan module 'kyber_common' is useful only when enabling modules 'kyber', 'kyber_90s' or both");
#endif

namespace Botan {

class BOTAN_PUBLIC_API(3, 0) KyberMode {
   public:
      enum Mode {
         // Kyber512 as proposed in round 3 of the NIST competition
         Kyber512_R3,
         // Kyber768 as proposed in round 3 of the NIST competition
         Kyber768_R3,
         // Kyber1024 as proposed in round 3 of the NIST competition
         Kyber1024_R3,

         Kyber512 BOTAN_DEPRECATED("Use Kyber512_R3") = Kyber512_R3,
         Kyber768 BOTAN_DEPRECATED("Use Kyber768_R3") = Kyber768_R3,
         Kyber1024 BOTAN_DEPRECATED("Use Kyber1024_R3") = Kyber1024_R3,

         Kyber512_90s BOTAN_DEPRECATED("Kyber 90s mode is deprecated"),
         Kyber768_90s BOTAN_DEPRECATED("Kyber 90s mode is deprecated"),
         Kyber1024_90s BOTAN_DEPRECATED("Kyber 90s mode is deprecated"),
      };

      KyberMode(Mode mode);
      explicit KyberMode(const OID& oid);
      explicit KyberMode(std::string_view str);

      OID object_identifier() const;
      std::string to_string() const;

      Mode mode() const { return m_mode; }

      BOTAN_DEPRECATED("Kyber 90s mode is deprecated") bool is_90s() const;

      BOTAN_DEPRECATED("Kyber 90s mode is deprecated") bool is_modern() const;

      bool is_kyber_round3() const;

      bool is_available() const;

      bool operator==(const KyberMode& other) const { return m_mode == other.m_mode; }

      bool operator!=(const KyberMode& other) const { return !(*this == other); }

   private:
      Mode m_mode;
};

class Kyber_PublicKeyInternal;
class Kyber_PrivateKeyInternal;

class BOTAN_PUBLIC_API(3, 0) Kyber_PublicKey : public virtual Public_Key {
   public:
      Kyber_PublicKey(std::span<const uint8_t> pub_key, KyberMode mode);

      Kyber_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      Kyber_PublicKey(const Kyber_PublicKey& other);

      Kyber_PublicKey& operator=(const Kyber_PublicKey& other) = default;

      ~Kyber_PublicKey() override = default;

      std::string algo_name() const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      OID object_identifier() const override;

      size_t key_length() const override;

      size_t estimated_strength() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;

      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator&, bool) const override;

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override {
         return (op == PublicKeyOperation::KeyEncapsulation);
      }

      std::unique_ptr<PK_Ops::KEM_Encryption> create_kem_encryption_op(std::string_view params,
                                                                       std::string_view provider) const override;

      KyberMode mode() const;

   protected:
      Kyber_PublicKey() = default;

      static std::shared_ptr<Kyber_PublicKeyInternal> initialize_from_encoding(std::span<const uint8_t> pub_key,
                                                                               KyberMode m);

   protected:
      friend class Kyber_KEM_Encryptor;
      friend class Kyber_KEM_Decryptor;

      std::shared_ptr<Kyber_PublicKeyInternal> m_public;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(3, 0) Kyber_PrivateKey final : public virtual Kyber_PublicKey,
                                                      public virtual Private_Key {
   public:
      Kyber_PrivateKey(RandomNumberGenerator& rng, KyberMode mode);

      Kyber_PrivateKey(std::span<const uint8_t> sk, KyberMode mode);

      Kyber_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      std::unique_ptr<Public_Key> public_key() const override;

      secure_vector<uint8_t> private_key_bits() const override;

      secure_vector<uint8_t> raw_private_key_bits() const override;

      std::unique_ptr<PK_Ops::KEM_Decryption> create_kem_decryption_op(RandomNumberGenerator& rng,
                                                                       std::string_view params,
                                                                       std::string_view provider) const override;

   private:
      friend class Kyber_KEM_Decryptor;

      std::shared_ptr<Kyber_PrivateKeyInternal> m_private;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
