/*
* (C) 2018 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PWDHASH_H_
#define BOTAN_PWDHASH_H_

#include <botan/types.h>
#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace Botan {

/**
* Base class for password based key derivation functions.
*
* Converts a password into a key using a salt and iterated hashing to
* make brute force attacks harder.
*/
class BOTAN_PUBLIC_API(2, 8) PasswordHash {
   public:
      virtual ~PasswordHash() = default;

      virtual std::string to_string() const = 0;

      /**
      * Most password hashes have some notion of iterations.
      */
      virtual size_t iterations() const = 0;

      /**
      * Some password hashing algorithms have a parameter which controls how
      * much memory is used. If not supported by some algorithm, returns 0.
      */
      virtual size_t memory_param() const { return 0; }

      /**
      * Some password hashing algorithms have a parallelism parameter.
      * If the algorithm does not support this notion, then the
      * function returns zero. This allows distinguishing between a
      * password hash which just does not support parallel operation,
      * vs one that does support parallel operation but which has been
      * configured to use a single lane.
      */
      virtual size_t parallelism() const { return 0; }

      /**
      * Returns an estimate of the total number of bytes required to perform this
      * key derivation.
      *
      * If this algorithm uses a small and constant amount of memory, with no
      * effort made towards being memory hard, this function returns 0.
      */
      virtual size_t total_memory_usage() const { return 0; }

      /**
      * Returns true if this password hash supports supplying a key
      */
      virtual bool supports_keyed_operation() const { return false; }

      /**
      * Returns true if this password hash supports supplying associated data
      */
      virtual bool supports_associated_data() const { return false; }

      /**
      * Hash a password into a bitstring
      *
      * @param out a span where the derived key will be placed
      * @param password the password to derive the key from
      * @param salt a randomly chosen salt
      *
      * This function is const, but is not thread safe. Different threads should
      * either use unique objects, or serialize all access.
      */
      void hash(std::span<uint8_t> out, std::string_view password, std::span<const uint8_t> salt) const {
         this->derive_key(out.data(), out.size(), password.data(), password.size(), salt.data(), salt.size());
      }

      /**
      * Hash a password into a bitstring
      *
      * @param out a span where the derived key will be placed
      * @param password the password to derive the key from
      * @param salt a randomly chosen salt
      * @param associated_data some additional data
      * @param key a secret key
      *
      * This function is const, but is not thread safe. Different threads should
      * either use unique objects, or serialize all access.
      */
      void hash(std::span<uint8_t> out,
                std::string_view password,
                std::span<const uint8_t> salt,
                std::span<const uint8_t> associated_data,
                std::span<const uint8_t> key) const {
         this->derive_key(out.data(),
                          out.size(),
                          password.data(),
                          password.size(),
                          salt.data(),
                          salt.size(),
                          associated_data.data(),
                          associated_data.size(),
                          key.data(),
                          key.size());
      }

      /**
      * Derive a key from a password
      *
      * @param out buffer to store the derived key, must be of out_len bytes
      * @param out_len the desired length of the key to produce
      * @param password the password to derive the key from
      * @param password_len the length of password in bytes
      * @param salt a randomly chosen salt
      * @param salt_len length of salt in bytes
      *
      * This function is const, but is not thread safe. Different threads should
      * either use unique objects, or serialize all access.
      */
      virtual void derive_key(uint8_t out[],
                              size_t out_len,
                              const char* password,
                              size_t password_len,
                              const uint8_t salt[],
                              size_t salt_len) const = 0;

      /**
      * Derive a key from a password plus additional data and/or a secret key
      *
      * Currently this is only supported for Argon2. Using a non-empty AD or key
      * with other algorithms will cause a Not_Implemented exception.
      *
      * @param out buffer to store the derived key, must be of out_len bytes
      * @param out_len the desired length of the key to produce
      * @param password the password to derive the key from
      * @param password_len the length of password in bytes
      * @param salt a randomly chosen salt
      * @param salt_len length of salt in bytes
      * @param ad some additional data
      * @param ad_len length of ad in bytes
      * @param key a secret key
      * @param key_len length of key in bytes
      *
      * This function is const, but is not thread safe. Different threads should
      * either use unique objects, or serialize all access.
      */
      virtual void derive_key(uint8_t out[],
                              size_t out_len,
                              const char* password,
                              size_t password_len,
                              const uint8_t salt[],
                              size_t salt_len,
                              const uint8_t ad[],
                              size_t ad_len,
                              const uint8_t key[],
                              size_t key_len) const;
};

class BOTAN_PUBLIC_API(2, 8) PasswordHashFamily {
   public:
      /**
      * Create an instance based on a name
      * If provider is empty then best available is chosen.
      * @param algo_spec algorithm name
      * @param provider provider implementation to choose
      * @return a null pointer if the algo/provider combination cannot be found
      */
      static std::unique_ptr<PasswordHashFamily> create(std::string_view algo_spec, std::string_view provider = "");

      /**
      * Create an instance based on a name, or throw if the
      * algo/provider combination cannot be found. If provider is
      * empty then best available is chosen.
      */
      static std::unique_ptr<PasswordHashFamily> create_or_throw(std::string_view algo_spec,
                                                                 std::string_view provider = "");

      /**
      * @return list of available providers for this algorithm, empty if not available
      */
      static std::vector<std::string> providers(std::string_view algo_spec);

      virtual ~PasswordHashFamily() = default;

      /**
      * @return name of this PasswordHash
      */
      virtual std::string name() const = 0;

      /**
      * Return a new parameter set tuned for this machine
      * @param output_length how long the output length will be
      * @param msec the desired execution time in milliseconds
      *
      * @param max_memory_usage_mb some password hash functions can use a tunable
      * amount of memory, in this case max_memory_usage limits the amount of RAM
      * the returned parameters will require, in mebibytes (2**20 bytes). It may
      * require some small amount above the request. Set to zero to place no
      * limit at all.
      * @param tuning_msec how long to run the tuning loop
      */
      virtual std::unique_ptr<PasswordHash> tune(
         size_t output_length,
         std::chrono::milliseconds msec,
         size_t max_memory_usage_mb = 0,
         std::chrono::milliseconds tuning_msec = std::chrono::milliseconds(10)) const = 0;

      /**
      * Return some default parameter set for this PBKDF that should be good
      * enough for most users. The value returned may change over time as
      * processing power and attacks improve.
      */
      virtual std::unique_ptr<PasswordHash> default_params() const = 0;

      /**
      * Return a parameter chosen based on a rough approximation with the
      * specified iteration count. The exact value this returns for a particular
      * algorithm may change from over time. Think of it as an alternative to
      * tune, where time is expressed in terms of PBKDF2 iterations rather than
      * milliseconds.
      */
      virtual std::unique_ptr<PasswordHash> from_iterations(size_t iterations) const = 0;

      /**
      * Create a password hash using some scheme specific format. Parameters are as follows:
      * - For PBKDF2, PGP-S2K, and Bcrypt-PBKDF, i1 is iterations
      * - Scrypt uses N, r, p for i{1-3}
      * - Argon2 family uses memory (in KB), iterations, and parallelism for i{1-3}
      *
      * All unneeded parameters should be set to 0 or left blank.
      */
      virtual std::unique_ptr<PasswordHash> from_params(size_t i1, size_t i2 = 0, size_t i3 = 0) const = 0;
};

}  // namespace Botan

#endif
