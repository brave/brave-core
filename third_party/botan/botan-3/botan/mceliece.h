/*
 * (C) Copyright Projet SECRET, INRIA, Rocquencourt
 * (C) Bhaskar Biswas and  Nicolas Sendrier
 *
 * (C) 2014 cryptosource GmbH
 * (C) 2014 Falko Strenzke fstrenzke@cryptosource.de
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 *
 */

#ifndef BOTAN_MCELIECE_KEY_H_
#define BOTAN_MCELIECE_KEY_H_

#include <botan/pk_keys.h>

BOTAN_DEPRECATED_HEADER("mceliece.h")

namespace Botan {

typedef uint16_t gf2m;

class polyn_gf2m;

class BOTAN_PUBLIC_API(2, 0) McEliece_PublicKey : public virtual Public_Key {
   public:
      explicit McEliece_PublicKey(std::span<const uint8_t> key_bits);

      McEliece_PublicKey(const std::vector<uint8_t>& pub_matrix, size_t t, size_t the_code_length) :
            m_public_matrix(pub_matrix), m_t(t), m_code_length(the_code_length) {}

      McEliece_PublicKey(const McEliece_PublicKey& other) = default;
      McEliece_PublicKey& operator=(const McEliece_PublicKey& other) = default;
      ~McEliece_PublicKey() override = default;

      secure_vector<uint8_t> random_plaintext_element(RandomNumberGenerator& rng) const;

      std::string algo_name() const override { return "McEliece"; }

      AlgorithmIdentifier algorithm_identifier() const override;

      size_t key_length() const override;
      size_t estimated_strength() const override;

      std::vector<uint8_t> raw_public_key_bits() const override;
      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator&, bool) const override { return true; }

      size_t get_t() const { return m_t; }

      size_t get_code_length() const { return m_code_length; }

      size_t get_message_word_bit_length() const;

      const std::vector<uint8_t>& get_public_matrix() const { return m_public_matrix; }

      bool operator==(const McEliece_PublicKey& other) const;

      bool operator!=(const McEliece_PublicKey& other) const { return !(*this == other); }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override {
         return (op == PublicKeyOperation::KeyEncapsulation);
      }

      std::unique_ptr<PK_Ops::KEM_Encryption> create_kem_encryption_op(std::string_view params,
                                                                       std::string_view provider) const override;

   protected:
      McEliece_PublicKey() : m_t(0), m_code_length(0) {}

      std::vector<uint8_t> m_public_matrix;
      size_t m_t;
      size_t m_code_length;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) McEliece_PrivateKey final : public virtual McEliece_PublicKey,
                                                         public virtual Private_Key {
   public:
      /**
      Generate a McEliece key pair

      Suggested parameters for a given security level (SL)

      SL=80 n=1632 t=33 - 59 KB pubkey 140 KB privkey
      SL=107 n=2480 t=45 - 128 KB pubkey 300 KB privkey
      SL=128 n=2960 t=57 - 195 KB pubkey 459 KB privkey
      SL=147 n=3408 t=67 - 265 KB pubkey 622 KB privkey
      SL=191 n=4624 t=95 - 516 KB pubkey 1234 KB privkey
      SL=256 n=6624 t=115 - 942 KB pubkey 2184 KB privkey
      */
      McEliece_PrivateKey(RandomNumberGenerator& rng, size_t code_length, size_t t);

      explicit McEliece_PrivateKey(std::span<const uint8_t> key_bits);

      McEliece_PrivateKey(const polyn_gf2m& goppa_polyn,
                          const std::vector<uint32_t>& parity_check_matrix_coeffs,
                          const std::vector<polyn_gf2m>& square_root_matrix,
                          const std::vector<gf2m>& inverse_support,
                          const std::vector<uint8_t>& public_matrix);

      ~McEliece_PrivateKey() override;

      McEliece_PrivateKey(const McEliece_PrivateKey&);
      McEliece_PrivateKey& operator=(const McEliece_PrivateKey&);

      McEliece_PrivateKey(McEliece_PrivateKey&&) noexcept;
      McEliece_PrivateKey& operator=(McEliece_PrivateKey&&) noexcept;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      const polyn_gf2m& get_goppa_polyn() const;

      const std::vector<uint32_t>& get_H_coeffs() const { return m_coeffs; }

      const std::vector<gf2m>& get_Linv() const { return m_Linv; }

      const std::vector<polyn_gf2m>& get_sqrtmod() const { return m_sqrtmod; }

      inline size_t get_dimension() const { return m_dimension; }

      inline size_t get_codimension() const { return m_codimension; }

      secure_vector<uint8_t> private_key_bits() const override;

      std::unique_ptr<Public_Key> public_key() const override;

      bool operator==(const McEliece_PrivateKey& other) const;

      bool operator!=(const McEliece_PrivateKey& other) const { return !(*this == other); }

      std::unique_ptr<PK_Ops::KEM_Decryption> create_kem_decryption_op(RandomNumberGenerator& rng,
                                                                       std::string_view params,
                                                                       std::string_view provider) const override;

   private:
      std::vector<polyn_gf2m> m_g;  // single element
      std::vector<polyn_gf2m> m_sqrtmod;
      std::vector<gf2m> m_Linv;
      std::vector<uint32_t> m_coeffs;

      size_t m_codimension;
      size_t m_dimension;
};

BOTAN_DIAGNOSTIC_POP

/**
* Estimate work factor for McEliece
* @return estimated security level for these key parameters
*/
BOTAN_PUBLIC_API(2, 0) size_t mceliece_work_factor(size_t code_size, size_t t);

}  // namespace Botan

#endif
