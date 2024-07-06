/*
* ECKCDSA (ISO/IEC 14888-3:2018)
* (C) 2016 René Korthaus, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECKCDSA_KEY_H_
#define BOTAN_ECKCDSA_KEY_H_

#include <botan/ecc_key.h>

namespace Botan {

/**
* This class represents ECKCDSA public keys.
*/
class BOTAN_PUBLIC_API(2, 0) ECKCDSA_PublicKey : public virtual EC_PublicKey {
   public:
      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      ECKCDSA_PublicKey(const EC_Group& dom_par, const EC_Point& public_point) : EC_PublicKey(dom_par, public_point) {}

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      ECKCDSA_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits) :
            EC_PublicKey(alg_id, key_bits) {}

      /**
      * Get this keys algorithm name.
      */
      std::string algo_name() const override { return "ECKCDSA"; }

      size_t message_parts() const override { return 2; }

      size_t message_part_size() const override { return domain().get_order_bytes(); }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Signature); }

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

   protected:
      ECKCDSA_PublicKey() = default;
};

/**
* This class represents ECKCDSA private keys.
*/

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) ECKCDSA_PrivateKey final : public ECKCDSA_PublicKey,
                                                        public EC_PrivateKey {
   public:
      /**
      * Load a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits ECPrivateKey bits
      */
      ECKCDSA_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits) :
            EC_PrivateKey(alg_id, key_bits, true) {}

      /**
      * Create a private key.
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key (if zero, generate a new random key)
      */
      ECKCDSA_PrivateKey(RandomNumberGenerator& rng, const EC_Group& domain, const BigInt& x = BigInt::zero()) :
            EC_PrivateKey(rng, domain, x, true) {}

      bool check_key(RandomNumberGenerator& rng, bool) const override;

      std::unique_ptr<Public_Key> public_key() const override;

      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator& rng,
                                                             std::string_view params,
                                                             std::string_view provider) const override;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
