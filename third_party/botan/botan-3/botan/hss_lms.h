/*
 * Hierarchical Signature System (HSS) / Leighton-Micali Signature (LMS)
 * hash-based signature algorithm (RFC 8554).
 *
 * (C) 2023 Jack Lloyd
 *     2023 Philippe Lieser, Fabian Albert - Rohde & Schwarz Cybersecurity GmbH
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_HSS_LMS_H_
#define BOTAN_HSS_LMS_H_

#include <botan/pk_keys.h>

#include <memory>
#include <vector>

namespace Botan {

class HSS_LMS_PublicKeyInternal;
class HSS_LMS_PrivateKeyInternal;

/**
 * @brief An HSS/LMS public key.
 *
 * Implementation of the Hierarchical Signature System (HSS) of
 * Leighton-Micali Hash-Based Signatures (LMS) defined in RFC 8554
 * (https://www.rfc-editor.org/rfc/rfc8554.html).
 *
 * To derive seeds for single LMS trees in the HSS-multitree, the method (SECRET_METHOD 2)
 * of the reference implementation (https://github.com/cisco/hash-sigs) is used.
 */
class BOTAN_PUBLIC_API(3, 5) HSS_LMS_PublicKey : public virtual Public_Key {
   public:
      /**
       * @brief Load an existing public key using its bytes.
       */
      HSS_LMS_PublicKey(std::span<const uint8_t> pub_key_bytes);

      ~HSS_LMS_PublicKey() override;

      size_t key_length() const override;

      std::string algo_name() const override;

      size_t estimated_strength() const override;
      AlgorithmIdentifier algorithm_identifier() const override;
      OID object_identifier() const override;
      bool check_key(RandomNumberGenerator& rng, bool strong) const override;
      std::vector<uint8_t> raw_public_key_bits() const override;
      std::vector<uint8_t> public_key_bits() const override;

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& signature_algorithm,
                                                                        std::string_view provider) const override;

      bool supports_operation(PublicKeyOperation op) const override;

      /**
       * @throws Not_Implemented for LMS public keys.
       */
      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const override;

   protected:
      HSS_LMS_PublicKey() = default;

      std::shared_ptr<HSS_LMS_PublicKeyInternal> m_public;
};

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

/**
 * @brief An HSS/LMS private key.
 *
 * HSS/LMS is a statefule hash-based signature scheme. This means the private key must
 * be (securely) updated after using it for signing. Also, there is a maximal number
 * of signatures that can be created using one HSS/LMS key pair, which depends on
 * the number and size of LMS layers of the chosen HSS/LMS instance. For the selection
 * of a sensible parameter set, refer to RFC 8554 6.4.
 *
 * The format of the HSS/LMS private key is not defined in
 * RFC 8554. We use the following format (big endian):
 *
 * PrivateKey = u32str(L) || u64str(idx) ||
 *              u32str(LMS algorithm id (root layer)) || u32str(LMOTS algorithm id (root layer)) ||
 *              ... ||
 *              u32str(LMS algorithm id (bottom layer)) || u32str(LMOTS algorithm id (bottom layer)) ||
 *              HSS_SEED || HSS_Identifier
 *
 *  L: Number of LMS layers
 *  Idx: Number of signatures already created using this private key
 *  HSS_SEED: Seed to derive LMS Seeds (see RFC 8554 Appendix A) like in SECRET_METHOD 2 of
 *            https://github.com/cisco/hash-sigs. As long as the hash functions output length.
 *  HSS_Identifier: 16 bytes long.
 *
 * The HSS/LMS instance to use for creating new keys is defined using an algorithm parameter sting,
 * i.e. to define which hash function (hash), LMS tree height (h)
 * and OTS Winternitz coefficient widths (w) to use. The syntax is the following:
 *
 * HSS-LMS(<hash>,HW(<h>,<w>),HW(<h>,<w>),...)
 *
 * e.g. 'HSS-LMS(SHA-256,HW(5,1),HW(5,1))' to use SHA-256 in a two-layer HSS instance
 * with a LMS tree hights 5 and w=1. The following parameters are allowed (which are
 * specified in RFC 8554 and draft-fluhrer-lms-more-parm-sets-11):
 *
 * hash: 'SHA-256', 'Truncated(SHA-256,192)', 'SHAKE-256(256)', SHAKE-256(192)
 * h: '5', '10', '15', '20', '25'
 * w: '1', '2', '4', '8'
 *
 * Note: The selected hash function is also used for seed derivation.
 */
class BOTAN_PUBLIC_API(3, 5) HSS_LMS_PrivateKey final : public virtual HSS_LMS_PublicKey,
                                                        public virtual Private_Key {
   public:
      /**
       * @brief Load an existing LMS private key using its bytes
       */
      HSS_LMS_PrivateKey(std::span<const uint8_t> private_key_bytes);

      /**
       * @brief Construct a new hss lms privatekey object.
       *
       * @param rng random number generator
       * @param algo_params string is format 'HSS-LMS(<hash>,HW(<h>,<w>),HW(<h>,<w>),...)'
       */
      HSS_LMS_PrivateKey(RandomNumberGenerator& rng, std::string_view algo_params);

      ~HSS_LMS_PrivateKey() override;

      secure_vector<uint8_t> private_key_bits() const override;
      secure_vector<uint8_t> raw_private_key_bits() const override;
      std::unique_ptr<Public_Key> public_key() const override;

      AlgorithmIdentifier pkcs8_algorithm_identifier() const override;

      bool stateful_operation() const override { return true; }

      /**
       * Retrieves the number of remaining signatures for this private key.
       */
      std::optional<uint64_t> remaining_operations() const override;

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const override;

      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator& rng,
                                                             std::string_view params,
                                                             std::string_view provider) const override;

   private:
      HSS_LMS_PrivateKey(std::shared_ptr<HSS_LMS_PrivateKeyInternal> sk);

      std::shared_ptr<HSS_LMS_PrivateKeyInternal> m_private;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
