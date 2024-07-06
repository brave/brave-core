/*
* Cryptobox Message Routines
* (C) 2009 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CRYPTOBOX_H_
#define BOTAN_CRYPTOBOX_H_

#include <botan/symkey.h>
#include <string>

BOTAN_DEPRECATED_HEADER("cryptobox.h")

namespace Botan {

class RandomNumberGenerator;

/**
* This namespace holds various high-level crypto functions
*/
namespace CryptoBox {

/**
* Encrypt a message using a passphrase
* @param input the input data
* @param input_len the length of input in bytes
* @param passphrase the passphrase used to encrypt the message
* @param rng a ref to a random number generator, such as AutoSeeded_RNG
*/
BOTAN_DEPRECATED("CryptoBox interface is deprecated")
BOTAN_PUBLIC_API(2, 0) std::string
   encrypt(const uint8_t input[], size_t input_len, std::string_view passphrase, RandomNumberGenerator& rng);

/**
* Decrypt a message encrypted with CryptoBox::encrypt
* @param input the input data
* @param input_len the length of input in bytes
* @param passphrase the passphrase used to encrypt the message
*/
BOTAN_DEPRECATED("CryptoBox interface is deprecated")
BOTAN_PUBLIC_API(2, 3) secure_vector<uint8_t> decrypt_bin(const uint8_t input[],
                                                          size_t input_len,
                                                          std::string_view passphrase);

/**
* Decrypt a message encrypted with CryptoBox::encrypt
* @param input the input data
* @param passphrase the passphrase used to encrypt the message
*/
BOTAN_DEPRECATED("CryptoBox interface is deprecated")
BOTAN_PUBLIC_API(2, 3) secure_vector<uint8_t> decrypt_bin(std::string_view input, std::string_view passphrase);

/**
* Decrypt a message encrypted with CryptoBox::encrypt
* @param input the input data
* @param input_len the length of input in bytes
* @param passphrase the passphrase used to encrypt the message
*/
BOTAN_DEPRECATED("CryptoBox interface is deprecated")
BOTAN_PUBLIC_API(2, 0) std::string decrypt(const uint8_t input[], size_t input_len, std::string_view passphrase);

/**
* Decrypt a message encrypted with CryptoBox::encrypt
* @param input the input data
* @param passphrase the passphrase used to encrypt the message
*/
BOTAN_DEPRECATED("CryptoBox interface is deprecated")
BOTAN_PUBLIC_API(2, 0) std::string decrypt(std::string_view input, std::string_view passphrase);

}  // namespace CryptoBox

}  // namespace Botan

#endif
