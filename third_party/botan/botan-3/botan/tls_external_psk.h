/*
 * TLS 1.3 Preshared Key Container
 * (C) 2023 Jack Lloyd
 *     2023 Fabian Albert, René Meusel - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_TLS_EXTERNAL_PSK_H_
#define BOTAN_TLS_EXTERNAL_PSK_H_

#include <botan/secmem.h>
#include <botan/strong_type.h>

#include <utility>
#include <vector>

namespace Botan::TLS {

/**
 * This is an externally provided PreSharedKey along with its identity, master
 * secret and (in case of TLS 1.3) a pre-provisioned Pseudo Random Function.
 */
class ExternalPSK {
   public:
      ExternalPSK(const ExternalPSK&) = delete;
      ExternalPSK& operator=(const ExternalPSK&) = delete;
      ExternalPSK(ExternalPSK&&) = default;
      ExternalPSK& operator=(ExternalPSK&&) = default;
      ~ExternalPSK() = default;

      ExternalPSK(std::string_view identity, std::string_view prf_algo, secure_vector<uint8_t> psk) :
            m_identity(identity), m_prf_algo(prf_algo), m_master_secret(std::move(psk)) {}

      /**
       * Identity (e.g. username of the PSK owner) of the preshared key.
       * Despite the std::string return type, this may or may not be a
       * human-readable/printable string.
       */
      const std::string& identity() const { return m_identity; }

      /**
       * Returns the master secret by moving it out of this object. Do not call
       * this method more than once.
       */
      secure_vector<uint8_t> extract_master_secret() {
         BOTAN_STATE_CHECK(!m_master_secret.empty());
         return std::exchange(m_master_secret, {});
      }

      /**
       * External preshared keys in TLS 1.3 must be provisioned with a
       * pseudo-random function (typically SHA-256 or the like). This is
       * needed to calculate/verify the PSK binder values in the client hello.
       */
      const std::string& prf_algo() const { return m_prf_algo; }

   private:
      std::string m_identity;
      std::string m_prf_algo;
      secure_vector<uint8_t> m_master_secret;
};

}  // namespace Botan::TLS

#endif
