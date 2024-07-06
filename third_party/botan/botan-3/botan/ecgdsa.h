/*
* ECGDSA (BSI-TR-03111, version 2.0)
* (C) 2016 René Korthaus
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECGDSA_KEY_H_
#define BOTAN_ECGDSA_KEY_H_

#include <botan/ecc_key.h>

namespace Botan {

/**
* This class represents ECGDSA public keys.
*/
class BOTAN_PUBLIC_API(2, 0) ECGDSA_PublicKey : public virtual EC_PublicKey {
   public:
      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      ECGDSA_PublicKey(const EC_Group& dom_par, const EC_Point& public_point) : EC_PublicKey(dom_par, public_point) {}

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      ECGDSA_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits) :
            EC_PublicKey(alg_id, key_bits) {}

      /**
      * Get this keys algorithm name.
      * @result this keys algorithm name ("ECGDSA")
      */
      std::string algo_name() const override { return "ECGDSA"; }

      size_t message_parts() const override { return 2; }

      size_t message_part_size() const override { return domain().get_order_bytes(); }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Signature); }

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

   protected:
      ECGDSA_PublicKey() = default;
};

/**
* This class represents ECGDSA private keys.
*/

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) ECGDSA_PrivateKey final : public ECGDSA_PublicKey,
                                                       public EC_PrivateKey {
   public:
      /**
      * Load a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits ECPrivateKey bits
      */
      ECGDSA_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits) :
            EC_PrivateKey(alg_id, key_bits, true) {}

      /**
      * Generate a new private key.
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key (if zero, generate a new random key)
      */
      ECGDSA_PrivateKey(RandomNumberGenerator& rng, const EC_Group& domain, const BigInt& x = BigInt::zero()) :
            EC_PrivateKey(rng, domain, x, true) {}

      std::unique_ptr<Public_Key> public_key() const override;

      bool check_key(RandomNumberGenerator& rng, bool) const override;

      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator& rng,
                                                             std::string_view params,
                                                             std::string_view provider) const override;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
