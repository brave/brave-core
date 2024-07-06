/*
* OpenPGP PBKDF
* (C) 1999-2007,2017 Jack Lloyd
* (C) 2018 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_OPENPGP_S2K_H_
#define BOTAN_OPENPGP_S2K_H_

#include <botan/hash.h>
#include <botan/pbkdf.h>
#include <botan/pwdhash.h>
#include <botan/rfc4880.h>

// Use pwdhash.h
BOTAN_FUTURE_INTERNAL_HEADER(pgp_s2k.h)

namespace Botan {

/**
* OpenPGP's S2K
*
* See RFC 4880 sections 3.7.1.1, 3.7.1.2, and 3.7.1.3
* If the salt is empty and iterations == 1, "simple" S2K is used
* If the salt is non-empty and iterations == 1, "salted" S2K is used
* If the salt is non-empty and iterations > 1, "iterated" S2K is used
*
* Due to complexities of the PGP S2K algorithm, time-based derivation
* is not supported. So if iterations == 0 and msec.count() > 0, an
* exception is thrown. In the future this may be supported, in which
* case "iterated" S2K will be used and the number of iterations
* performed is returned.
*
* Note that unlike PBKDF2, OpenPGP S2K's "iterations" are defined as
* the number of bytes hashed.
*/
class BOTAN_PUBLIC_API(2, 2) OpenPGP_S2K final : public PBKDF {
   public:
      /**
      * @param hash the hash function to use
      */
      explicit OpenPGP_S2K(std::unique_ptr<HashFunction> hash) : m_hash(std::move(hash)) {}

      std::string name() const override { return "OpenPGP-S2K(" + m_hash->name() + ")"; }

      std::unique_ptr<PBKDF> new_object() const override { return std::make_unique<OpenPGP_S2K>(m_hash->new_object()); }

      size_t pbkdf(uint8_t output_buf[],
                   size_t output_len,
                   std::string_view passphrase,
                   const uint8_t salt[],
                   size_t salt_len,
                   size_t iterations,
                   std::chrono::milliseconds msec) const override;

      /**
      * RFC 4880 encodes the iteration count to a single-byte value
      */
      static uint8_t encode_count(size_t iterations) { return RFC4880_encode_count(iterations); }

      static size_t decode_count(uint8_t encoded_iter) { return RFC4880_decode_count(encoded_iter); }

   private:
      std::unique_ptr<HashFunction> m_hash;
};

/**
* OpenPGP's S2K
*
* See RFC 4880 sections 3.7.1.1, 3.7.1.2, and 3.7.1.3
* If the salt is empty and iterations == 1, "simple" S2K is used
* If the salt is non-empty and iterations == 1, "salted" S2K is used
* If the salt is non-empty and iterations > 1, "iterated" S2K is used
*
* Note that unlike PBKDF2, OpenPGP S2K's "iterations" are defined as
* the number of bytes hashed.
*/
class BOTAN_PUBLIC_API(2, 8) RFC4880_S2K final : public PasswordHash {
   public:
      /**
      * @param hash the hash function to use
      * @param iterations is rounded due to PGP formatting
      */
      RFC4880_S2K(std::unique_ptr<HashFunction> hash, size_t iterations);

      std::string to_string() const override;

      size_t iterations() const override { return m_iterations; }

      void derive_key(uint8_t out[],
                      size_t out_len,
                      const char* password,
                      size_t password_len,
                      const uint8_t salt[],
                      size_t salt_len) const override;

   private:
      std::unique_ptr<HashFunction> m_hash;
      size_t m_iterations;
};

class BOTAN_PUBLIC_API(2, 8) RFC4880_S2K_Family final : public PasswordHashFamily {
   public:
      RFC4880_S2K_Family(std::unique_ptr<HashFunction> hash) : m_hash(std::move(hash)) {}

      std::string name() const override;

      std::unique_ptr<PasswordHash> tune(size_t output_len,
                                         std::chrono::milliseconds msec,
                                         size_t max_mem,
                                         std::chrono::milliseconds tune_msec) const override;

      /**
      * Return some default parameter set for this PBKDF that should be good
      * enough for most users. The value returned may change over time as
      * processing power and attacks improve.
      */
      std::unique_ptr<PasswordHash> default_params() const override;

      std::unique_ptr<PasswordHash> from_iterations(size_t iter) const override;

      std::unique_ptr<PasswordHash> from_params(size_t iter, size_t, size_t) const override;

   private:
      std::unique_ptr<HashFunction> m_hash;
};

}  // namespace Botan

#endif
