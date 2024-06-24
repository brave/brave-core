/*
 * XMSS Keys
 * (C) 2016,2017 Matthias Gierlings
 * (C) 2019 René Korthaus, Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_H_
#define BOTAN_XMSS_H_

#include <memory>
#include <span>

#include <botan/exceptn.h>
#include <botan/pk_keys.h>
#include <botan/xmss_parameters.h>

namespace Botan {

class RandomNumberGenerator;
class XMSS_Address;
class XMSS_Hash;
class XMSS_PrivateKey_Internal;
class XMSS_Verification_Operation;
class XMSS_WOTS_PublicKey;
class XMSS_WOTS_PrivateKey;

/**
 * An XMSS: Extended Hash-Based Signature public key.
 *
 * [1] XMSS: Extended Hash-Based Signatures,
 *     Request for Comments: 8391
 *     Release: May 2018.
 *     https://datatracker.ietf.org/doc/rfc8391/
 **/
class BOTAN_PUBLIC_API(2, 0) XMSS_PublicKey : public virtual Public_Key {
   public:
      /**
       * Creates a new XMSS public key for the chosen XMSS signature method.
       * New public and prf seeds are generated using rng. The appropriate WOTS
       * signature method will be automatically set based on the chosen XMSS
       * signature method.
       *
       * @param xmss_oid Identifier for the selected XMSS signature method.
       * @param rng A random number generator to use for key generation.
       **/
      XMSS_PublicKey(XMSS_Parameters::xmss_algorithm_t xmss_oid, RandomNumberGenerator& rng);

      /**
       * Loads a public key.
       *
       * Public key must be encoded as in RFC
       * draft-vangeest-x509-hash-sigs-03.
       *
       * @param key_bits DER encoded public key bits
       */
      XMSS_PublicKey(std::span<const uint8_t> key_bits);

      /**
       * Creates a new XMSS public key for a chosen XMSS signature method as
       * well as pre-computed root node and public_seed values.
       *
       * @param xmss_oid Identifier for the selected XMSS signature method.
       * @param root Root node value.
       * @param public_seed Public seed value.
       **/
      XMSS_PublicKey(XMSS_Parameters::xmss_algorithm_t xmss_oid,
                     secure_vector<uint8_t> root,
                     secure_vector<uint8_t> public_seed);

      std::string algo_name() const override { return "XMSS"; }

      AlgorithmIdentifier algorithm_identifier() const override {
         return AlgorithmIdentifier(object_identifier(), AlgorithmIdentifier::USE_EMPTY_PARAM);
      }

      bool check_key(RandomNumberGenerator&, bool) const override { return true; }

      size_t estimated_strength() const override { return m_xmss_params.estimated_strength(); }

      size_t key_length() const override { return m_xmss_params.estimated_strength(); }

      /**
       * Generates a byte sequence representing the XMSS
       * public key, as defined in [1] (p. 23, "XMSS Public Key")
       *
       * @return 4-byte OID, followed by n-byte root node, followed by
       *         public seed.
       **/
      std::vector<uint8_t> raw_public_key_bits() const override;

      /**
       * Returns the encoded public key as defined in RFC
       * draft-vangeest-x509-hash-sigs-03.
       *
       * @return encoded public key bits
       **/
      std::vector<uint8_t> public_key_bits() const override;

      BOTAN_DEPRECATED("Use raw_public_key_bits()") std::vector<uint8_t> raw_public_key() const;

      std::unique_ptr<Private_Key> generate_another(RandomNumberGenerator& rng) const final;

      bool supports_operation(PublicKeyOperation op) const override { return (op == PublicKeyOperation::Signature); }

      std::unique_ptr<PK_Ops::Verification> create_verification_op(std::string_view params,
                                                                   std::string_view provider) const override;

      std::unique_ptr<PK_Ops::Verification> create_x509_verification_op(const AlgorithmIdentifier& alg_id,
                                                                        std::string_view provider) const override;

   protected:
      friend class XMSS_Verification_Operation;

      const secure_vector<uint8_t>& public_seed() const { return m_public_seed; }

      const secure_vector<uint8_t>& root() const { return m_root; }

      const XMSS_Parameters& xmss_parameters() const { return m_xmss_params; }

   protected:
      std::vector<uint8_t> m_raw_key;
      XMSS_Parameters m_xmss_params;
      XMSS_WOTS_Parameters m_wots_params;
      secure_vector<uint8_t> m_root;
      secure_vector<uint8_t> m_public_seed;
};

template <typename>
class Atomic;

class XMSS_Index_Registry;

/**
 * Determines how WOTS+ private keys are derived from the XMSS private key
 */
enum class WOTS_Derivation_Method {
   /// This roughly followed the suggestions in RFC 8391 but is vulnerable
   /// to a multi-target attack. For new private keys, we recommend using
   /// the derivation as suggested in NIST SP.800-208.
   /// Private keys generated with Botan 2.x will need to stay with this mode,
   /// otherwise they won't be able to generate valid signatures any longer.
   Botan2x = 1,

   /// Derivation as specified in NIST SP.800-208 to avoid a multi-target attack
   /// on the WOTS+ key derivation suggested in RFC 8391. New private keys
   /// should use this mode.
   NIST_SP800_208 = 2,
};

/**
 * An XMSS: Extended Hash-Based Signature private key.
 * The XMSS private key does not support the X509 and PKCS7 standard. Instead
 * the raw format described in [1] is used.
 *
 * [1] XMSS: Extended Hash-Based Signatures,
 *     Request for Comments: 8391
 *     Release: May 2018.
 *     https://datatracker.ietf.org/doc/rfc8391/
 **/

BOTAN_DIAGNOSTIC_PUSH
BOTAN_DIAGNOSTIC_IGNORE_INHERITED_VIA_DOMINANCE

class BOTAN_PUBLIC_API(2, 0) XMSS_PrivateKey final : public virtual XMSS_PublicKey,
                                                     public virtual Private_Key {
   public:
      /**
      * Creates a new XMSS private key for the chosen XMSS signature method.
      * New seeds for public/private key and pseudo random function input are
      * generated using the provided RNG. The appropriate WOTS signature method
      * will be automatically set based on the chosen XMSS signature method.
      *
      * @param xmss_algo_id Identifier for the selected XMSS signature method.
      * @param rng A random number generator to use for key generation.
      * @param wots_derivation_method The method used to derive WOTS+ private keys
      **/
      XMSS_PrivateKey(XMSS_Parameters::xmss_algorithm_t xmss_algo_id,
                      RandomNumberGenerator& rng,
                      WOTS_Derivation_Method wots_derivation_method = WOTS_Derivation_Method::NIST_SP800_208);

      /**
       * Creates an XMSS_PrivateKey from a byte sequence produced by
       * raw_private_key().
       *
       * @param raw_key An XMSS private key serialized using raw_private_key().
       **/
      XMSS_PrivateKey(std::span<const uint8_t> raw_key);

      /**
       * Creates a new XMSS private key for the chosen XMSS signature method
       * using precomputed seeds for public/private keys and pseudo random
       * function input. The appropriate WOTS signature method will be
       * automatically set, based on the chosen XMSS signature method.
       *
       * @param xmss_algo_id Identifier for the selected XMSS signature method.
       * @param idx_leaf Index of the next unused leaf.
       * @param wots_priv_seed A seed to generate a Winternitz-One-Time-
       *                      Signature private key from.
       * @param prf a secret n-byte key sourced from a secure source
       *        of uniformly random data.
       * @param root Root node of the binary hash tree.
       * @param public_seed The public seed.
       * @param wots_derivation_method The method used to derive WOTS+ private keys
       **/
      XMSS_PrivateKey(XMSS_Parameters::xmss_algorithm_t xmss_algo_id,
                      size_t idx_leaf,
                      secure_vector<uint8_t> wots_priv_seed,
                      secure_vector<uint8_t> prf,
                      secure_vector<uint8_t> root,
                      secure_vector<uint8_t> public_seed,
                      WOTS_Derivation_Method wots_derivation_method = WOTS_Derivation_Method::NIST_SP800_208);

      bool stateful_operation() const override { return true; }

      std::unique_ptr<Public_Key> public_key() const override;

      /**
       * Retrieves the last unused leaf index of the private key. Reusing a leaf
       * by utilizing leaf indices lower than the last unused leaf index will
       * compromise security.
       *
       * @return Index of the last unused leaf.
       **/
      BOTAN_DEPRECATED("Use remaining_operations()") size_t unused_leaf_index() const;

      /**
       * Retrieves the number of remaining signatures for this private key.
       */
      BOTAN_DEPRECATED("Use remaining_operations()") size_t remaining_signatures() const;

      std::optional<uint64_t> remaining_operations() const override;

      std::unique_ptr<PK_Ops::Signature> create_signature_op(RandomNumberGenerator&,
                                                             std::string_view,
                                                             std::string_view provider) const override;

      secure_vector<uint8_t> private_key_bits() const override;

      /**
       * Generates a non standartized byte sequence representing the XMSS
       * private key.
       *
       * @return byte sequence consisting of the following elements in order:
       *         4-byte OID, n-byte root node, n-byte public seed,
       *         8-byte unused leaf index, n-byte prf seed, n-byte private seed.
       **/
      secure_vector<uint8_t> raw_private_key() const;

      WOTS_Derivation_Method wots_derivation_method() const;

   private:
      friend class XMSS_Signature_Operation;

      size_t reserve_unused_leaf_index();

      const secure_vector<uint8_t>& prf_value() const;

      XMSS_WOTS_PublicKey wots_public_key_for(XMSS_Address& adrs, XMSS_Hash& hash) const;
      XMSS_WOTS_PrivateKey wots_private_key_for(XMSS_Address& adrs, XMSS_Hash& hash) const;

      /**
       * Algorithm 9: "treeHash"
       * Computes the internal n-byte nodes of a Merkle tree.
       *
       * @param start_idx The start index.
       * @param target_node_height Height of the target node.
       * @param adrs Address of the tree containing the target node.
       *
       * @return The root node of a tree of height target_node height with the
       *         leftmost leaf being the hash of the WOTS+ pk with index
       *         start_idx.
       **/
      secure_vector<uint8_t> tree_hash(size_t start_idx, size_t target_node_height, XMSS_Address& adrs);

      void tree_hash_subtree(secure_vector<uint8_t>& result,
                             size_t start_idx,
                             size_t target_node_height,
                             XMSS_Address& adrs);

      /**
       * Helper for multithreaded tree hashing.
       */
      void tree_hash_subtree(secure_vector<uint8_t>& result,
                             size_t start_idx,
                             size_t target_node_height,
                             XMSS_Address& adrs,
                             XMSS_Hash& hash);

      std::shared_ptr<XMSS_PrivateKey_Internal> m_private;
};

BOTAN_DIAGNOSTIC_POP

}  // namespace Botan

#endif
