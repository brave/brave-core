/*
* Random Number Generator base classes
* (C) 1999-2009,2015,2016 Jack Lloyd
*     2023                René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RANDOM_NUMBER_GENERATOR_H_
#define BOTAN_RANDOM_NUMBER_GENERATOR_H_

#include <botan/concepts.h>
#include <botan/exceptn.h>
#include <botan/mutex.h>
#include <botan/secmem.h>

#include <chrono>
#include <concepts>
#include <span>
#include <string>
#include <type_traits>

namespace Botan {

class Entropy_Sources;

/**
* An interface to a cryptographic random number generator
*/
class BOTAN_PUBLIC_API(2, 0) RandomNumberGenerator {
   public:
      virtual ~RandomNumberGenerator() = default;

      RandomNumberGenerator() = default;

      /*
      * Never copy a RNG, create a new one
      */
      RandomNumberGenerator(const RandomNumberGenerator& rng) = delete;
      RandomNumberGenerator& operator=(const RandomNumberGenerator& rng) = delete;

      /**
      * Randomize a byte array.
      *
      * May block shortly if e.g. the RNG is not yet initialized
      * or a retry because of insufficient entropy is needed.
      *
      * @param output the byte array to hold the random output.
      * @throws PRNG_Unseeded if the RNG fails because it has not enough entropy
      * @throws Exception if the RNG fails
      */
      void randomize(std::span<uint8_t> output) { this->fill_bytes_with_input(output, {}); }

      void randomize(uint8_t output[], size_t length) { this->randomize(std::span(output, length)); }

      /**
      * Returns false if it is known that this RNG object is not able to accept
      * externally provided inputs (via add_entropy, randomize_with_input, etc).
      * In this case, any such provided inputs are ignored.
      *
      * If this function returns true, then inputs may or may not be accepted.
      */
      virtual bool accepts_input() const = 0;

      /**
      * Incorporate some additional data into the RNG state. For
      * example adding nonces or timestamps from a peer's protocol
      * message can help hedge against VM state rollback attacks.
      * A few RNG types do not accept any externally provided input,
      * in which case this function is a no-op.
      *
      * @param input a byte array containing the entropy to be added
      * @throws Exception may throw if the RNG accepts input, but adding the entropy failed.
      */
      void add_entropy(std::span<const uint8_t> input) { this->fill_bytes_with_input({}, input); }

      void add_entropy(const uint8_t input[], size_t length) { this->add_entropy(std::span(input, length)); }

      /**
      * Incorporate some additional data into the RNG state.
      */
      template <typename T>
         requires std::is_standard_layout<T>::value && std::is_trivial<T>::value
      void add_entropy_T(const T& t) {
         this->add_entropy(reinterpret_cast<const uint8_t*>(&t), sizeof(T));
      }

      /**
      * Incorporate entropy into the RNG state then produce output.
      * Some RNG types implement this using a single operation, default
      * calls add_entropy + randomize in sequence.
      *
      * Use this to further bind the outputs to your current
      * process/protocol state. For instance if generating a new key
      * for use in a session, include a session ID or other such
      * value. See NIST SP 800-90 A, B, C series for more ideas.
      *
      * @param output buffer to hold the random output
      * @param input entropy buffer to incorporate
      * @throws PRNG_Unseeded if the RNG fails because it has not enough entropy
      * @throws Exception if the RNG fails
      * @throws Exception may throw if the RNG accepts input, but adding the entropy failed.
      */
      void randomize_with_input(std::span<uint8_t> output, std::span<const uint8_t> input) {
         this->fill_bytes_with_input(output, input);
      }

      void randomize_with_input(uint8_t output[], size_t output_len, const uint8_t input[], size_t input_len) {
         this->randomize_with_input(std::span(output, output_len), std::span(input, input_len));
      }

      /**
      * This calls `randomize_with_input` using some timestamps as extra input.
      *
      * For a stateful RNG using non-random but potentially unique data the
      * extra input can help protect against problems with fork, VM state
      * rollback, or other cases where somehow an RNG state is duplicated. If
      * both of the duplicated RNG states later incorporate a timestamp (and the
      * timestamps don't themselves repeat), their outputs will diverge.
      *
      * @param output buffer to hold the random output
      * @throws PRNG_Unseeded if the RNG fails because it has not enough entropy
      * @throws Exception if the RNG fails
      * @throws Exception may throw if the RNG accepts input, but adding the entropy failed.
      */
      void randomize_with_ts_input(std::span<uint8_t> output);

      void randomize_with_ts_input(uint8_t output[], size_t output_len) {
         this->randomize_with_ts_input(std::span(output, output_len));
      }

      /**
      * @return the name of this RNG type
      */
      virtual std::string name() const = 0;

      /**
      * Clear all internally held values of this RNG
      * @post is_seeded() == false if the RNG has an internal state that can be cleared.
      */
      virtual void clear() = 0;

      /**
      * Check whether this RNG is seeded.
      * @return true if this RNG was already seeded, false otherwise.
      */
      virtual bool is_seeded() const = 0;

      /**
      * Poll provided sources for up to poll_bits bits of entropy
      * or until the timeout expires. Returns estimate of the number
      * of bits collected.
      *
      * Sets the seeded state to true if enough entropy was added.
      */
      virtual size_t reseed(Entropy_Sources& srcs,
                            size_t poll_bits = BOTAN_RNG_RESEED_POLL_BITS,
                            std::chrono::milliseconds poll_timeout = BOTAN_RNG_RESEED_DEFAULT_TIMEOUT);

      /**
      * Reseed by reading specified bits from the RNG
      *
      * Sets the seeded state to true if enough entropy was added.
      *
      * @throws Exception if RNG accepts input but reseeding failed.
      */
      virtual void reseed_from_rng(RandomNumberGenerator& rng, size_t poll_bits = BOTAN_RNG_RESEED_POLL_BITS);

      // Some utility functions built on the interface above:

      /**
      * Fill a given byte container with @p bytes random bytes
      *
      * @todo deprecate this overload (in favor of randomize())
      *
      * @param  v     the container to be filled with @p bytes random bytes
      * @throws Exception if RNG fails
      */
      void random_vec(std::span<uint8_t> v) { this->randomize(v); }

      /**
      * Resize a given byte container to @p bytes and fill it with random bytes
      *
      * @tparam T     the desired byte container type (e.g std::vector<uint8_t>)
      * @param  v     the container to be filled with @p bytes random bytes
      * @param  bytes number of random bytes to initialize the container with
      * @throws Exception if RNG or memory allocation fails
      */
      template <concepts::resizable_byte_buffer T>
      void random_vec(T& v, size_t bytes) {
         v.resize(bytes);
         random_vec(v);
      }

      /**
      * Create some byte container type and fill it with some random @p bytes.
      *
      * @tparam T     the desired byte container type (e.g std::vector<uint8_t>)
      * @param  bytes number of random bytes to initialize the container with
      * @return       a container of type T with @p bytes random bytes
      * @throws Exception if RNG or memory allocation fails
      */
      template <concepts::resizable_byte_buffer T = secure_vector<uint8_t>>
         requires std::default_initializable<T>
      T random_vec(size_t bytes) {
         T result;
         random_vec(result, bytes);
         return result;
      }

      /**
      * Return a random byte
      * @return random byte
      * @throws PRNG_Unseeded if the RNG fails because it has not enough entropy
      * @throws Exception if the RNG fails
      */
      uint8_t next_byte() {
         uint8_t b;
         this->fill_bytes_with_input(std::span(&b, 1), {});
         return b;
      }

      /**
      * @return a random byte that is greater than zero
      * @throws PRNG_Unseeded if the RNG fails because it has not enough entropy
      * @throws Exception if the RNG fails
      */
      uint8_t next_nonzero_byte() {
         uint8_t b = this->next_byte();
         while(b == 0) {
            b = this->next_byte();
         }
         return b;
      }

   protected:
      /**
      * Generic interface to provide entropy to a concrete implementation and to
      * fill a given buffer with random output. Both @p output and @p input may
      * be empty and should be ignored in that case. If both buffers are
      * non-empty implementations should typically first apply the @p input data
      * and then generate random data into @p output.
      *
      * This method must be implemented by all RandomNumberGenerator sub-classes.
      *
      * @param output  Byte buffer to write random bytes into. Implementations
      *                should not read from this buffer.
      * @param input   Byte buffer that may contain bytes to be incorporated in
      *                the RNG's internal state. Implementations may choose to
      *                ignore the bytes in this buffer.
      */
      virtual void fill_bytes_with_input(std::span<uint8_t> output, std::span<const uint8_t> input) = 0;
};

/**
* Convenience typedef
*/
typedef RandomNumberGenerator RNG;

/**
* Hardware_RNG exists to tag hardware RNG types (PKCS11_RNG, TPM_RNG, Processor_RNG)
*/
class BOTAN_PUBLIC_API(2, 0) Hardware_RNG : public RandomNumberGenerator {
   public:
      void clear() final { /* no way to clear state of hardware RNG */
      }
};

/**
* Null/stub RNG - fails if you try to use it for anything
* This is not generally useful except for in certain tests
*/
class BOTAN_PUBLIC_API(2, 0) Null_RNG final : public RandomNumberGenerator {
   public:
      bool is_seeded() const override { return false; }

      bool accepts_input() const override { return false; }

      void clear() override {}

      std::string name() const override { return "Null_RNG"; }

   private:
      void fill_bytes_with_input(std::span<uint8_t> output, std::span<const uint8_t> /* ignored */) override;
};

}  // namespace Botan

#endif
