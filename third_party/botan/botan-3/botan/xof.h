/*
* Extendable Output Function Base Class
* (C) 2023 Jack Lloyd
*     2023 Fabian Albert, René Meusel - Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_XOF_BASE_CLASS_H_
#define BOTAN_XOF_BASE_CLASS_H_

#include <botan/concepts.h>
#include <botan/secmem.h>
#include <botan/sym_algo.h>

#include <memory>
#include <string>
#include <string_view>

namespace Botan {

/**
 * This class represents an eXtendable Output Function (XOF) objects
 *
 * A XOF transforms an arbitrary length input message into an indefinite
 * stream of output bits. Typically, it is illegal to call `update()` after
 * the first call to `output()`.
 */
class BOTAN_PUBLIC_API(3, 2) XOF {
   public:
      XOF() : m_xof_started(false) {}

      virtual ~XOF() = default;

      /**
       * Create an instance based on a name, or return null if the
       * algo/provider combination cannot be found. If provider is
       * empty then best available is chosen.
       */
      static std::unique_ptr<XOF> create(std::string_view algo_spec, std::string_view provider = "");

      /**
       * Create an instance based on a name
       * If provider is empty then best available is chosen.
       * @param algo_spec algorithm name
       * @param provider provider implementation to use
       * Throws Lookup_Error if not found.
       */
      static std::unique_ptr<XOF> create_or_throw(std::string_view algo_spec, std::string_view provider = "");

      /**
       * @return list of available providers for this algorithm, empty if not available
       * @param algo_spec algorithm name
       */
      static std::vector<std::string> providers(std::string_view algo_spec);

      /**
       * @return provider information about this implementation. Default is "base",
       * might also return "sse2", "avx2", "openssl", or some other arbitrary string.
       */
      virtual std::string provider() const;

      /**
       * Reset the state.
       */
      void clear() {
         m_xof_started = false;
         reset();
      }

      /**
       * @return the hash function name
       */
      virtual std::string name() const = 0;

      /**
       * Some XOFs can be parameterized with a @p salt and/or @p key. If required,
       * this must be called before calling XOF::update().
       *
       * @sa XOF::valid_salt_length()
       * @sa XOF::key_spec()
       *
       * @param salt  a salt value to parameterize the XOF
       * @param key   a key to parameterize the XOF
       */
      void start(std::span<const uint8_t> salt = {}, std::span<const uint8_t> key = {});

      /**
       * @returns true if salt length is acceptable, false otherwise
       */
      virtual bool valid_salt_length(size_t salt_len) const {
         // Salts are not supported by default
         return salt_len == 0;
      }

      /**
       * @returns an object describing limits on the key size
       */
      virtual Key_Length_Specification key_spec() const {
         // Keys are not supported by default
         return Key_Length_Specification(0);
      }

      /**
       * @return the intrinsic processing block size of this XOF
       */
      virtual size_t block_size() const = 0;

      /**
       * Return a new XOF object with the same state as *this.
       *
       * If the XOF is not yet in the output phase, it efficiently allows
       * using several messages with a common prefix.
       * Otherwise, the copied state will produce the same output
       * bit stream as the original object at the time of this invocation.
       *
       * This function should be called `clone` but for consistency with
       * other classes it is called `copy_state`.
       *
       * @return new XOF object
       */
      virtual std::unique_ptr<XOF> copy_state() const = 0;

      /**
       * @return new object representing the same algorithm as *this
       */
      virtual std::unique_ptr<XOF> new_object() const = 0;

      /**
       * Typically, this is `true` for new objects and becomes `false`
       * once `output()` was called for the first time.
       *
       * @returns true iff calling `update()` is legal in the current object state
       */
      virtual bool accepts_input() const = 0;

      /**
       * Add @p input data to the XOF's internal state
       *
       * @param input  the data that shall be
       */
      void update(std::span<const uint8_t> input) {
         if(!m_xof_started) {
            // If the user didn't start() before the first input, we enforce
            // it with a default value, here.
            start();
         }
         add_data(input);
      }

      /**
       * @return the next @p bytes output bytes as the specified container type @p T.
       */
      template <concepts::resizable_byte_buffer T = secure_vector<uint8_t>>
      T output(size_t bytes) {
         T out(bytes);
         generate_bytes(out);
         return out;
      }

      /**
       * Convenience overload to generate a std::vector<uint8_t>. Same as calling
       * `XOF::output<std::vector<uint8_t>>()`.
       *
       * @return the next @p bytes output bytes as a byte vector.
       */
      std::vector<uint8_t> output_stdvec(size_t bytes) { return output<std::vector<uint8_t>>(bytes); }

      /**
       * Fill @p output with the next output bytes. The number of bytes
       * depends on the size of @p output.
       */
      void output(std::span<uint8_t> output) { generate_bytes(output); }

      /**
       * @return the next single output byte
       */
      uint8_t output_next_byte() {
         uint8_t out;
         generate_bytes({&out, 1});
         return out;
      }

   private:
      /**
       * Take @p salt and/or @p key to pre-parameterize the XOF. This must be called
       * before calling XOF::update().
       *
       * @param salt  a salt value to parameterize the XOF
       * @param key   a key to parameterize the XOF
       */
      virtual void start_msg(std::span<const uint8_t> salt, std::span<const uint8_t> key);

      /**
       * Consume @p input data bytes into the XOF's internal state
       *
       * Typically, XOFs may consume an arbitrary length of input data but
       * should refuse accepting more input once the first output bit was
       * generated. Implementations should throw `Invalid_State` in this
       * case.
       *
       * @param input  the span to be consumed entirely into the internal state
       * @throws        Invalid_State if input is added after generating output
       */
      virtual void add_data(std::span<const uint8_t> input) = 0;

      /**
       * Fill the entire @p output span with the next bytes in their output
       * stream.
       *
       * The first invocation to `generate_bytes()` should typically transition
       * the XOF's state to "output mode" and prevent any further calls to
       * `XOF::add_data()`.
       *
       * @param output  the span to be filled entirely with output bytes
       */
      virtual void generate_bytes(std::span<uint8_t> output) = 0;

      /**
       * Clear the XOF's internal state and allow for new input.
       */
      virtual void reset() = 0;

   private:
      bool m_xof_started;
};

}  // namespace Botan

#endif
