/*
* GOST 34.10-2001
* (C) 2007 Falko Strenzke, FlexSecure GmbH
*          Manuel Hartl, FlexSecure GmbH
* (C) 2008-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_GOST_3410_KEY_H_
#define BOTAN_GOST_3410_KEY_H_

#include <botan/ecc_key.h>

namespace Botan {

/**
* GOST-34.10 Public Key
*/
class BOTAN_PUBLIC_API(2, 0) GOST_3410_PublicKey : public virtual EC_PublicKey {
   public:
      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      GOST_3410_PublicKey(const EC_Group& dom_par, const EC_Point& public_point) :
            EC_PublicKey(dom_par, public_point) {}

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      GOST_3410_PublicKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits);

      /**
      * Get this keys algorithm name.
      * @result this keys algorithm name
      */
      std::string algo_name() const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> public_key_bits() const override;

      size_t message_parts() const override { return 2; }

      size_t message_part_size() const override { return domain().get_order_bytes(); }

      Signature_Format default_x509_signature_format() const override { return Signature_Format::Standard; }

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Signature); }

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

   protected:
      GOST_3410_PublicKey() = default;
};

/**
* GOST-34.10 Private Key
*/

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) GOST_3410_PrivateKey final : public GOST_3410_PublicKey,
                                                          public EC_PrivateKey {
   public:
      /**
      * Load a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits ECPrivateKey bits
      */
      GOST_3410_PrivateKey(const AlgorithmIdentifier& alg_id, std::span<const uint8_t> key_bits) :
            EC_PrivateKey(alg_id, key_bits) {}

      /**
      * Generate a new private key
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key; if zero, a new random key is generated
      */
      GOST_3410_PrivateKey(RandomNumberGenerator& rng, const EC_Group& domain, const BigInt& x = BigInt::zero());

      std::unique_ptr<Public_Key> public_key() const override;

      AlgorithmIdentifier pkcs8_algorithm_identifier() const override { return EC_PublicKey::algorithm_identifier(); }

      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator& rng,
                                                             std::string_view params,
                                                             std::string_view provider) const override;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
