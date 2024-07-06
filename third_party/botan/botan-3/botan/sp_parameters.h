/*
 * SPHINCS+ Parameters
 * (C) 2023 Jack Lloyd
 *     2023 Fabian Albert, René Meusel, Amos Treiber - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_SP_PARAMS_H_
#define BOTAN_SP_PARAMS_H_

#include <botan/asn1_obj.h>

#include <string_view>

namespace Botan {

enum class Sphincs_Hash_Type {
   Shake256,
   Sha256,
   Haraka,  ///< Haraka is currently not supported
};

enum class Sphincs_Parameter_Set {
   Sphincs128Small,
   Sphincs128Fast,
   Sphincs192Small,
   Sphincs192Fast,
   Sphincs256Small,
   Sphincs256Fast,
};

/**
 * Container for all SPHINCS+ parameters defined by a specific instance (see
 * Table 3 of Sphincs+ round 3.1 spec). Also contains getters for various
 * parameters that are derived from the given parameters.
 */
class BOTAN_PUBLIC_API(3, 1) Sphincs_Parameters final {
   public:
      static Sphincs_Parameters create(Sphincs_Parameter_Set set, Sphincs_Hash_Type hash);
      static Sphincs_Parameters create(std::string_view name);
      static Sphincs_Parameters create(const OID& oid);

      /**
       * @returns the OID of the algorithm specified by those parameters
       */
      OID object_identifier() const;

      /**
       * @returns the algorithm specifier for the selected parameter set
       */
      AlgorithmIdentifier algorithm_identifier() const;

      /**
       * @returns the hash type used by those parameters
       */
      Sphincs_Hash_Type hash_type() const { return m_hash_type; }

      /**
       * @returns the generic algorithm parameterization set to be used by those parameters
       */
      Sphincs_Parameter_Set parameter_set() const { return m_set; }

      /**
       * @returns a string representation of this parameter set
       */
      std::string to_string() const;

      /**
       * @returns the algorithm specifier of the hash function to be used
       */
      std::string hash_name() const;

      /**
       * @returns SPHINCS+ security parameter in bytes
       */
      uint32_t n() const { return m_n; }

      /**
       * @returns Height of the SPHINCS+ hypertree
       */
      uint32_t h() const { return m_h; }

      /**
       * @returns Number of XMSS layers in the SPHINCS+ hypertree
       */
      uint32_t d() const { return m_d; }

      /**
       * This is the desired height of the FORS trees, aka `log(t)` with t being
       * the number of leaves in each FORS tree.
       *
       * @returns Height of the FORS trees
       */
      uint32_t a() const { return m_a; }

      /**
       * @returns Number of FORS trees to use
       */
      uint32_t k() const { return m_k; }

      /**
       * @returns the Winternitz parameter for WOTS+ signatures
       */
      uint32_t w() const { return m_w; }

      /**
       * @returns the bit security given by Table 3 (NIST R3.1 submission, page 39) for the
       *          selected parameter set
       */
      uint32_t bitsec() const { return m_bitsec; }

      /**
       * @returns the tree height of an XMSS tree
       */
      uint32_t xmss_tree_height() const { return m_xmss_tree_height; }

      /**
       * @returns the byte length of a single xmss signature
       */
      uint32_t xmss_signature_bytes() const { return m_xmss_sig_bytes; }

      /**
       * @returns the byte length of a the xmss hypertree signature
       */
      uint32_t ht_signature_bytes() const { return m_ht_sig_bytes; }

      /**
       * @returns the base 2 logarithm of the Winternitz parameter for WOTS+ signatures
       */
      uint32_t log_w() const { return m_log_w; }

      /**
       * @returns the len1 parameter for WOTS+ signatures
       */
      uint32_t wots_len_1() const { return m_wots_len1; }

      /**
       * @returns the len2 parameter for WOTS+ signatures
       */
      uint32_t wots_len_2() const { return m_wots_len2; }

      /**
       * @returns the len parameter for WOTS+ signatures
       */
      uint32_t wots_len() const { return m_wots_len; }

      /**
       * @returns the byte length of a WOTS+ signature
       */
      uint32_t wots_bytes() const { return m_wots_bytes; }

      /**
       * @returns the number of bytes a WOTS+ signature consists of
       */
      uint32_t wots_checksum_bytes() const { return m_wots_checksum_bytes; }

      /**
       * @returns the byte length of a FORS signature
       */
      uint32_t fors_signature_bytes() const { return m_fors_sig_bytes; }

      /**
       * @returns the byte length of the FORS input message
       */
      uint32_t fors_message_bytes() const { return m_fors_message_bytes; }

      /**
       * @returns the byte length of a Sphincs+ signature
       */
      uint32_t sphincs_signature_bytes() const { return m_sp_sig_bytes; }

      /**
       * @returns the byte length of an encoded public key for this parameter set
       */
      uint32_t public_key_bytes() const { return m_n * 2; }

      /**
       * @returns the byte length of an encoded private key for this parameter set
       */
      uint32_t private_key_bytes() const { return m_n * 2 + public_key_bytes(); }

      /**
       * @returns the byte length of the tree index output of H_msg
       */
      uint32_t tree_digest_bytes() const { return m_tree_digest_bytes; }

      /**
       * @returns the byte length of the leaf index output of H_msg
       */
      uint32_t leaf_digest_bytes() const { return m_leaf_digest_bytes; }

      /**
       * @returns the byte length of the output of H_msg. Corresponds to
       *          m in the specification of H_msg in Section 7.2
       */
      uint32_t h_msg_digest_bytes() const { return m_h_msg_digest_bytes; }

   private:
      Sphincs_Parameters(Sphincs_Parameter_Set set,
                         Sphincs_Hash_Type hash_type,
                         uint32_t n,
                         uint32_t h,
                         uint32_t d,
                         uint32_t a,
                         uint32_t k,
                         uint32_t w,
                         uint32_t bitsec);

   private:
      Sphincs_Parameter_Set m_set;
      Sphincs_Hash_Type m_hash_type;
      uint32_t m_n;
      uint32_t m_h;
      uint32_t m_d;
      uint32_t m_a;
      uint32_t m_k;
      uint32_t m_w;
      uint32_t m_bitsec;
      uint32_t m_log_w;
      uint32_t m_wots_len1;
      uint32_t m_wots_len2;
      uint32_t m_wots_len;
      uint32_t m_wots_bytes;
      uint32_t m_wots_checksum_bytes;
      uint32_t m_fors_message_bytes;
      uint32_t m_fors_sig_bytes;
      uint32_t m_sp_sig_bytes;
      uint32_t m_xmss_tree_height;
      uint32_t m_xmss_sig_bytes;
      uint32_t m_ht_sig_bytes;

      uint32_t m_tree_digest_bytes;
      uint32_t m_leaf_digest_bytes;
      uint32_t m_h_msg_digest_bytes;
};

}  // namespace Botan

#endif
