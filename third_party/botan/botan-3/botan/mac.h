/*
* Base class for message authentiction codes
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MESSAGE_AUTH_CODE_BASE_H_
#define BOTAN_MESSAGE_AUTH_CODE_BASE_H_

#include <botan/buf_comp.h>
#include <botan/sym_algo.h>
#include <memory>
#include <span>
#include <string>

namespace Botan {

/**
* This class represents Message Authentication Code (MAC) objects.
*/
class BOTAN_PUBLIC_API(2, 0) MessageAuthenticationCode : public Buffered_Computation,
                                                         public SymmetricAlgorithm {
   public:
      /**
      * Create an instance based on a name
      * If provider is empty then best available is chosen.
      * @param algo_spec algorithm name
      * @param provider provider implementation to use
      * @return a null pointer if the algo/provider combination cannot be found
      */
      static std::unique_ptr<MessageAuthenticationCode> create(std::string_view algo_spec,
                                                               std::string_view provider = "");

      /*
      * Create an instance based on a name
      * If provider is empty then best available is chosen.
      * @param algo_spec algorithm name
      * @param provider provider implementation to use
      * Throws a Lookup_Error if algo/provider combination cannot be found
      */
      static std::unique_ptr<MessageAuthenticationCode> create_or_throw(std::string_view algo_spec,
                                                                        std::string_view provider = "");

      /**
      * @return list of available providers for this algorithm, empty if not available
      */
      static std::vector<std::string> providers(std::string_view algo_spec);

      ~MessageAuthenticationCode() override = default;

      /**
      * Prepare for processing a message under the specified nonce
      *
      * Most MACs neither require nor support a nonce; for these algorithms
      * calling `start_msg` is optional and calling it with anything other than
      * an empty string is an error. One MAC which *requires* a per-message
      * nonce be specified is GMAC.
      *
      * Default implementation simply rejects all non-empty nonces
      * since most hash/MAC algorithms do not support randomization
      *
      * @param nonce the message nonce bytes
      */
      void start(std::span<const uint8_t> nonce) { start_msg(nonce); }

      /**
      * Begin processing a message.
      * @param nonce the per message nonce
      * @param nonce_len length of nonce
      */
      void start(const uint8_t nonce[], size_t nonce_len) { start_msg({nonce, nonce_len}); }

      /**
      * Begin processing a message.
      */
      void start() { return start_msg({}); }

      /**
      * Verify a MAC.
      * @param in the MAC to verify as a byte array
      * @param length the length of param in
      * @return true if the MAC is valid, false otherwise
      */
      bool verify_mac(const uint8_t in[], size_t length) { return verify_mac_result({in, length}); }

      /**
      * Verify a MAC.
      * @param in the MAC to verify as a byte array
      * @return true if the MAC is valid, false otherwise
      */
      bool verify_mac(std::span<const uint8_t> in) { return verify_mac_result(in); }

      /**
      * @return new object representing the same algorithm as *this
      */
      virtual std::unique_ptr<MessageAuthenticationCode> new_object() const = 0;

      /**
      * Get a new object representing the same algorithm as *this
      */
      MessageAuthenticationCode* clone() const { return this->new_object().release(); }

      /**
      * @return provider information about this implementation. Default is "base",
      * might also return "sse2", "avx2", "openssl", or some other arbitrary string.
      */
      virtual std::string provider() const { return "base"; }

      /**
      * @return if a fresh key must be set for each message that is processed.
      *
      * This is required for certain polynomial-based MACs which are insecure
      * if a key is ever reused for two different messages.
      */
      virtual bool fresh_key_required_per_message() const { return false; }

   protected:
      /**
      * Prepare for processing a message under the specified nonce
      *
      * If the MAC does not support nonces, it should not override the default
      * implementation.
      */
      virtual void start_msg(std::span<const uint8_t> nonce);

      /**
      * Verify the MACs final result
      */
      virtual bool verify_mac_result(std::span<const uint8_t> in);
};

typedef MessageAuthenticationCode MAC;

}  // namespace Botan

#endif
