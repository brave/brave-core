/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <openssl/base64.h>
#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/sha.h>

#include <algorithm>

#include "bat/confirmations/internal/security_helper.h"

#include "base/base64.h"

#include "tweetnacl.h"  // NOLINT

namespace helper {

std::string Security::Sign(
    const std::map<std::string, std::string>& headers,
    const std::string& key_id,
    const std::vector<uint8_t>& private_key) {
  if (headers.empty() || key_id.empty() || private_key.empty()) {
    return "";
  }

  std::string concatenated_header = "";
  std::string concatenated_message = "";

  unsigned int index = 0;
  for (const auto& header : headers) {
    if (index != 0) {
      concatenated_header += " ";
      concatenated_message += "\n";
    }

    concatenated_header += header.first;
    concatenated_message += header.first + ": " + header.second;

    index++;
  }

  std::vector<uint8_t> signed_message(crypto_sign_BYTES +
      concatenated_message.length());

  // Resolving the following linter error breaks the build on Windows
  unsigned long long signed_message_size = 0;  // NOLINT
  crypto_sign(&signed_message.front(), &signed_message_size,
      reinterpret_cast<const unsigned char*>(concatenated_message.c_str()),
      concatenated_message.length(), &private_key.front());

  std::vector<uint8_t> signature(crypto_sign_BYTES);
  std::copy(signed_message.begin(), signed_message.begin() +
      crypto_sign_BYTES, signature.begin());

  return "keyId=\"" + key_id + "\",algorithm=\"" + crypto_sign_PRIMITIVE +
      "\",headers=\"" + concatenated_header + "\",signature=\"" +
      GetBase64(signature) + "\"";
}

std::vector<Token> Security::GenerateTokens(const int count) {
  DCHECK_GT(count, 0);

  std::vector<Token> tokens;

  for (int i = 0; i < count; i++) {
    auto token = Token::random();
    tokens.push_back(token);
  }

  return tokens;
}

std::vector<BlindedToken> Security::BlindTokens(
    const std::vector<Token>& tokens) {
  DCHECK_NE(tokens.size(), 0UL);

  std::vector<BlindedToken> blinded_tokens;
  for (unsigned int i = 0; i < tokens.size(); i++) {
    auto token = tokens.at(i);
    auto blinded_token = token.blind();

    blinded_tokens.push_back(blinded_token);
  }

  return blinded_tokens;
}

std::vector<uint8_t> Security::GetSHA256(const std::string& string) {
  if (string.empty()) {
    return {};
  }

  std::vector<uint8_t> string_sha256(SHA256_DIGEST_LENGTH);
  SHA256((uint8_t*)string.c_str(), string.length(), &string_sha256.front());
  return string_sha256;
}

std::string Security::GetBase64(const std::vector<uint8_t>& data) {
  DCHECK(!data.empty());

  size_t size = 0;
  if (!EVP_EncodedLength(&size, data.size())) {
    return "";
  }

  std::vector<uint8_t> string(size);
  int encoded_bytes_count =
      EVP_EncodeBlock(&string.front(), &data.front(), data.size());
  DCHECK_NE(encoded_bytes_count, 0);

  return std::string(reinterpret_cast<char*>(&string.front()));
}

}  // namespace helper
