/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "security_helper.h"

#include "base/base64.h"

#include "tweetnacl.h"
#include <openssl/base64.h>
#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/sha.h>

namespace helper {

std::string Security::Sign(
    const std::vector<std::string>& keys,
    const std::vector<std::string>& values,
    const unsigned int size,
    const std::string& key_id,
    const std::vector<uint8_t>& secret_key) {
  std::string headers = "";
  std::string message = "";

  for (unsigned i = 0; i < size; i++) {
    if (i != 0) {
      headers += " ";
      message += "\n";
    }

    headers += keys.at(i);
    message += keys.at(i) + ": " + values.at(i);
  }

  std::vector<uint8_t> signed_message(crypto_sign_BYTES + message.length());

  uint64_t signed_message_size = 0;
  crypto_sign(&signed_message.front(), &signed_message_size,
      (const unsigned char*)message.c_str(), (uint64_t)message.length(),
      &secret_key.front());

  std::vector<uint8_t> signature(crypto_sign_BYTES);
  std::copy(signed_message.begin(), signed_message.begin() +
      crypto_sign_BYTES, signature.begin());

  return "keyId=\"" + key_id + "\",algorithm=\"" + crypto_sign_PRIMITIVE +
      "\",headers=\"" + headers + "\",signature=\"" +
      GetBase64(signature) + "\"";
}

std::vector<Token> Security::GenerateTokens(const unsigned int count) {
  std::vector<Token> tokens;

  for (unsigned int i = 0; i < count; i++) {
    auto token = Token::random();
    tokens.push_back(token);
  }

  return tokens;
}

std::vector<BlindedToken> Security::BlindTokens(
    const std::vector<Token>& tokens) {
  std::vector<BlindedToken> blinded_tokens;
  for (unsigned int i = 0; i < tokens.size(); i++) {
    auto token = tokens.at(i);
    auto blinded_token = token.blind();

    blinded_tokens.push_back(blinded_token);
  }

  return blinded_tokens;
}

std::vector<uint8_t> Security::GetSHA256(const std::string& string) {
  std::vector<uint8_t> string_sha256(SHA256_DIGEST_LENGTH);
  SHA256((uint8_t*)string.c_str(), string.length(), &string_sha256.front());
  return string_sha256;
}

std::string Security::GetBase64(const std::vector<uint8_t>& data) {
  size_t size = 0;
  if (!EVP_EncodedLength(&size, data.size())) {
    DCHECK(false);
    return "";
  }

  std::vector<uint8_t> string(size);
  int num_encoded_bytes = EVP_EncodeBlock(
      &string.front(), &data.front(), data.size());
  DCHECK_NE(num_encoded_bytes, 0);
  return std::string(reinterpret_cast<char*>(&string.front()));
}

}  // namespace helper
