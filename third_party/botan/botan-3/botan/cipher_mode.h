/*
* Cipher Modes
* (C) 2013,2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CIPHER_MODE_H_
#define BOTAN_CIPHER_MODE_H_

#include <botan/concepts.h>
#include <botan/exceptn.h>
#include <botan/secmem.h>
#include <botan/sym_algo.h>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Botan {

/**
* The two possible directions for cipher filters, determining whether they
* actually perform encryption or decryption.
*/
enum class Cipher_Dir : int {
   Encryption,
   Decryption,

   ENCRYPTION BOTAN_DEPRECATED("Use Cipher_Dir::Encryption") = Encryption,
   DECRYPTION BOTAN_DEPRECATED("Use Cipher_Dir::Decryption") = Decryption,
};

/**
* Interface for cipher modes
*/
class BOTAN_PUBLIC_API(2, 0) Cipher_Mode : public SymmetricAlgorithm {
   public:
      /**
      * @return list of available providers for this algorithm, empty if not available
      * @param algo_spec algorithm name
      */
      static std::vector<std::string> providers(std::string_view algo_spec);

      /**
      * Create an AEAD mode
      * @param algo the algorithm to create
      * @param direction specify if this should be an encryption or decryption AEAD
      * @param provider optional specification for provider to use
      * @return an AEAD mode or a null pointer if not available
      */
      static std::unique_ptr<Cipher_Mode> create(std::string_view algo,
                                                 Cipher_Dir direction,
                                                 std::string_view provider = "");

      /**
      * Create an AEAD mode, or throw
      * @param algo the algorithm to create
      * @param direction specify if this should be an encryption or decryption AEAD
      * @param provider optional specification for provider to use
      * @return an AEAD mode, or throw an exception
      */
      static std::unique_ptr<Cipher_Mode> create_or_throw(std::string_view algo,
                                                          Cipher_Dir direction,
                                                          std::string_view provider = "");

   protected:
      /*
      * Prepare for processing a message under the specified nonce
      */
      virtual void start_msg(const uint8_t nonce[], size_t nonce_len) = 0;

      /*
      * Process message blocks
      * Input must be a multiple of update_granularity.
      */
      virtual size_t process_msg(uint8_t msg[], size_t msg_len) = 0;

      /*
      * Finishes a message
      */
      virtual void finish_msg(secure_vector<uint8_t>& final_block, size_t offset = 0) = 0;

   public:
      /**
      * Begin processing a message with a fresh nonce.
      * @param nonce the per message nonce
      */
      void start(std::span<const uint8_t> nonce) { start_msg(nonce.data(), nonce.size()); }

      /**
      * Begin processing a message with a fresh nonce.
      * @param nonce the per message nonce
      * @param nonce_len length of nonce
      */
      void start(const uint8_t nonce[], size_t nonce_len) { start_msg(nonce, nonce_len); }

      /**
      * Begin processing a message.
      *
      * The exact semantics of this depend on the mode. For many modes, the call
      * will fail since a nonce must be provided.
      *
      * For certain modes such as CBC this will instead cause the last
      * ciphertext block to be used as the nonce of the new message; doing this
      * isn't a good idea, but some (mostly older) protocols do this.
      */
      void start() { return start_msg(nullptr, 0); }

      /**
      * Process message blocks
      *
      * Input must be a multiple of update_granularity
      *
      * Processes msg in place and returns bytes written. Normally
      * this will be either msg_len (indicating the entire message was
      * processed) or for certain AEAD modes zero (indicating that the
      * mode requires the entire message be processed in one pass).
      *
      * @param msg the message to be processed
      * @return bytes written in-place
      */
      size_t process(std::span<uint8_t> msg) { return this->process_msg(msg.data(), msg.size()); }

      size_t process(uint8_t msg[], size_t msg_len) { return this->process_msg(msg, msg_len); }

      /**
      * Process some data. Input must be in size update_granularity() uint8_t blocks.
      * @param buffer in/out parameter which will possibly be resized
      * @param offset an offset into blocks to begin processing
      */
      template <concepts::resizable_byte_buffer T>
      void update(T& buffer, size_t offset = 0) {
         BOTAN_ASSERT(buffer.size() >= offset, "Offset ok");
         const size_t written = process(std::span(buffer).subspan(offset));
         buffer.resize(offset + written);
      }

      /**
      * Complete processing of a message.
      *
      * @param final_block in/out parameter which must be at least
      *        minimum_final_size() bytes, and will be set to any final output
      * @param offset an offset into final_block to begin processing
      */
      void finish(secure_vector<uint8_t>& final_block, size_t offset = 0) { finish_msg(final_block, offset); }

      /**
      * Complete procession of a message.
      *
      * Note: Using this overload with anything but a Botan::secure_vector<>
      *       is copying the bytes in the in/out buffer.
      *
      * @param final_block in/out parameter which must be at least
      *        minimum_final_size() bytes, and will be set to any final output
      * @param offset an offset into final_block to begin processing
      */
      template <concepts::resizable_byte_buffer T>
      void finish(T& final_block, size_t offset = 0) {
         Botan::secure_vector<uint8_t> tmp(final_block.begin(), final_block.end());
         finish_msg(tmp, offset);
         final_block.resize(tmp.size());
         std::copy(tmp.begin(), tmp.end(), final_block.begin());
      }

      /**
      * Returns the size of the output if this transform is used to process a
      * message with input_length bytes. In most cases the answer is precise.
      * If it is not possible to precise (namely for CBC decryption) instead an
      * upper bound is returned.
      */
      virtual size_t output_length(size_t input_length) const = 0;

      /**
      * @return size of required blocks to update
      */
      virtual size_t update_granularity() const = 0;

      /**
      * Return an ideal granularity. This will be a multiple of the result of
      * update_granularity but may be larger. If so it indicates that better
      * performance may be achieved by providing buffers that are at least that
      * size.
      */
      virtual size_t ideal_granularity() const = 0;

      /**
      * Certain modes require the entire message be available before
      * any processing can occur. For such modes, input will be consumed
      * but not returned, until `finish` is called, which returns the
      * entire message.
      *
      * This function returns true if this mode has this style of
      * operation.
      */
      virtual bool requires_entire_message() const { return false; }

      /**
      * @return required minimium size to finalize() - may be any
      *         length larger than this.
      */
      virtual size_t minimum_final_size() const = 0;

      /**
      * @return the default size for a nonce
      */
      virtual size_t default_nonce_length() const = 0;

      /**
      * @return true iff nonce_len is a valid length for the nonce
      */
      virtual bool valid_nonce_length(size_t nonce_len) const = 0;

      /**
      * Resets just the message specific state and allows encrypting again under the existing key
      */
      virtual void reset() = 0;

      /**
      * @return true iff this mode provides authentication as well as
      * confidentiality.
      */
      bool authenticated() const { return this->tag_size() > 0; }

      /**
      * @return the size of the authentication tag used (in bytes)
      */
      virtual size_t tag_size() const { return 0; }

      /**
      * @return provider information about this implementation. Default is "base",
      * might also return "sse2", "avx2", "openssl", or some other arbitrary string.
      */
      virtual std::string provider() const { return "base"; }
};

/**
* Get a cipher mode by name (eg "AES-128/CBC" or "Serpent/XTS")
* @param algo_spec cipher name
* @param direction Cipher_Dir::Encryption or Cipher_Dir::Decryption
* @param provider provider implementation to choose
*/
BOTAN_DEPRECATED("Use Cipher_Mode::create")
inline Cipher_Mode* get_cipher_mode(std::string_view algo_spec, Cipher_Dir direction, std::string_view provider = "") {
   return Cipher_Mode::create(algo_spec, direction, provider).release();
}

}  // namespace Botan

#endif
