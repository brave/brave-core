/*
* Stream Cipher
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_STREAM_CIPHER_H_
#define BOTAN_STREAM_CIPHER_H_

#include <botan/concepts.h>
#include <botan/sym_algo.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Botan {

/**
* Base class for all stream ciphers
*/
class BOTAN_PUBLIC_API(2, 0) StreamCipher : public SymmetricAlgorithm {
   public:
      ~StreamCipher() override = default;

      /**
      * Create an instance based on a name
      * If provider is empty then best available is chosen.
      * @param algo_spec algorithm name
      * @param provider provider implementation to use
      * @return a null pointer if the algo/provider combination cannot be found
      */
      static std::unique_ptr<StreamCipher> create(std::string_view algo_spec, std::string_view provider = "");

      /**
      * Create an instance based on a name
      * If provider is empty then best available is chosen.
      * @param algo_spec algorithm name
      * @param provider provider implementation to use
      * Throws a Lookup_Error if the algo/provider combination cannot be found
      */
      static std::unique_ptr<StreamCipher> create_or_throw(std::string_view algo_spec, std::string_view provider = "");

      /**
      * @return list of available providers for this algorithm, empty if not available
      */
      static std::vector<std::string> providers(std::string_view algo_spec);

      /**
      * Encrypt or decrypt a message
      * @param in the plaintext
      * @param out the byte array to hold the output, i.e. the ciphertext
      * @param len the length of both in and out in bytes
      */
      void cipher(const uint8_t in[], uint8_t out[], size_t len) { cipher_bytes(in, out, len); }

      /**
      * Encrypt or decrypt a message
      * @param in the plaintext
      * @param out the byte array to hold the output, i.e. the ciphertext
      *            with at least the same size as @p in
      */
      void cipher(std::span<const uint8_t> in, std::span<uint8_t> out) {
         BOTAN_ARG_CHECK(in.size() <= out.size(),
                         "Output buffer of stream cipher must be at least as long as input buffer");
         cipher_bytes(in.data(), out.data(), in.size());
      }

      /**
      * Write keystream bytes to a buffer
      *
      * The contents of @p out are ignored/overwritten
      *
      * @param out the byte array to hold the keystream
      * @param len the length of out in bytes
      */
      void write_keystream(uint8_t out[], size_t len) { generate_keystream(out, len); }

      /**
      * Fill a given buffer with keystream bytes
      *
      * The contents of @p out are ignored/overwritten
      *
      * @param out the byte array to hold the keystream
      */
      void write_keystream(std::span<uint8_t> out) { generate_keystream(out.data(), out.size()); }

      /**
      * Get @p bytes from the keystream
      *
      * @param bytes The number of bytes to be produced
      */
      template <concepts::resizable_byte_buffer T = secure_vector<uint8_t>>
      T keystream_bytes(size_t bytes) {
         T out(bytes);
         write_keystream(out);
         return out;
      }

      /**
      * Encrypt or decrypt a message
      * The message is encrypted/decrypted in place.
      * @param buf the plaintext / ciphertext
      * @param len the length of buf in bytes
      */
      void cipher1(uint8_t buf[], size_t len) { cipher(buf, buf, len); }

      /**
      * Encrypt or decrypt a message
      * The message is encrypted/decrypted in place.
      * @param buf the plaintext / ciphertext
      */
      void cipher1(std::span<uint8_t> buf) { cipher(buf, buf); }

      /**
      * Encrypt a message
      * The message is encrypted/decrypted in place.
      * @param inout the plaintext / ciphertext
      */
      void encipher(std::span<uint8_t> inout) { cipher(inout.data(), inout.data(), inout.size()); }

      /**
      * Encrypt a message
      * The message is encrypted in place.
      * @param inout the plaintext / ciphertext
      */
      void encrypt(std::span<uint8_t> inout) { cipher(inout.data(), inout.data(), inout.size()); }

      /**
      * Decrypt a message in place
      * The message is decrypted in place.
      * @param inout the plaintext / ciphertext
      */
      void decrypt(std::span<uint8_t> inout) { cipher(inout.data(), inout.data(), inout.size()); }

      /**
      * Return the optimium buffer size to use with this cipher
      *
      * Most stream ciphers internally produce blocks of bytes.  This function
      * returns that block size. Aligning buffer sizes to a multiple of this
      * size may improve performance by reducing internal buffering overhead.
      *
      * Note the return value of this function may change for any particular
      * algorithm due to changes in the implementation from release to release,
      * or changes in the runtime environment (such as CPUID indicating
      * availability of an optimized implementation). It is not intrinsic to
      * the algorithm; it is just a suggestion for gaining best performance.
      */
      virtual size_t buffer_size() const = 0;

      /**
      * Resync the cipher using the IV
      * @param iv the initialization vector
      * @param iv_len the length of the IV in bytes
      */
      void set_iv(const uint8_t iv[], size_t iv_len) { set_iv_bytes(iv, iv_len); }

      /**
      * Resync the cipher using the IV
      * @param iv the initialization vector
      */
      void set_iv(std::span<const uint8_t> iv) { set_iv_bytes(iv.data(), iv.size()); }

      /**
      * Return the default (preferred) nonce length
      * If this function returns 0, then this cipher does not support nonces
      *
      * Default implementation returns 0
      */
      virtual size_t default_iv_length() const;

      /**
      * @param iv_len the length of the IV in bytes
      * @return if the length is valid for this algorithm
      */
      virtual bool valid_iv_length(size_t iv_len) const { return (iv_len == 0); }

      /**
      * @return a new object representing the same algorithm as *this
      */
      StreamCipher* clone() const { return this->new_object().release(); }

      /**
      * @return new object representing the same algorithm as *this
      */
      virtual std::unique_ptr<StreamCipher> new_object() const = 0;

      /**
      * Set the offset and the state used later to generate the keystream
      * @param offset the offset where we begin to generate the keystream
      */
      virtual void seek(uint64_t offset) = 0;

      /**
      * @return provider information about this implementation. Default is "base",
      * might also return "sse2", "avx2" or some other arbitrary string.
      */
      virtual std::string provider() const { return "base"; }

   protected:
      /**
      * Encrypt or decrypt a message
      */
      virtual void cipher_bytes(const uint8_t in[], uint8_t out[], size_t len) = 0;

      /**
      * Write keystream bytes to a buffer
      */
      virtual void generate_keystream(uint8_t out[], size_t len);

      /**
      * Resync the cipher using the IV
      */
      virtual void set_iv_bytes(const uint8_t iv[], size_t iv_len) = 0;
};

}  // namespace Botan

#endif
