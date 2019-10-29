/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <openssl/base64.h>
#include <openssl/sha.h>

#include <vector>

#include "base/base64.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/common/security_helper.h"

namespace braveledger_helper {

std::vector<Token> Security::GenerateTokens(const int count) {
  DCHECK_GT(count, 0);
  std::vector<Token> tokens;

  for (auto i = 0; i < count; i++) {
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

std::string Security::GetBase64(const std::vector<uint8_t>& data) {
  DCHECK_NE(data.size(), 0UL);
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

std::vector<uint8_t> Security::GetSHA256(const std::string& string) {
  DCHECK(!string.empty());

  std::vector<uint8_t> string_sha256(SHA256_DIGEST_LENGTH);
  SHA256((uint8_t*)string.c_str(), string.length(), &string_sha256.front());
  return string_sha256;
}

std::string Security::Sign(
    const std::vector<std::map<std::string, std::string>>& headers,
    const std::string& key_id,
    const std::vector<uint8_t>& private_key) {
  DCHECK_NE(headers.size(), 0UL);
  DCHECK(!key_id.empty());
  DCHECK_NE(private_key.size(), 0UL);

  std::vector<std::string> header_keys;
  std::vector<std::string> header_values;

  for (const auto& header : headers) {
    const std::string key = header.begin()->first;
    const std::string value = header.begin()->second;

    header_keys.push_back(key);
    header_values.push_back(value);
  }

  std::vector<uint8_t> secret_key = braveledger_bat_helper::getHKDF(private_key);
  std::vector<uint8_t> public_key;
  std::vector<uint8_t> new_secret_key;
  bool success = braveledger_bat_helper::getPublicKeyFromSeed(
      secret_key,
      &public_key,
      &new_secret_key);
  if (!success) {
    return "";
  }

  return braveledger_bat_helper::sign(
      header_keys,
      header_values,
      key_id,
      new_secret_key);
}

}  // namespace braveledger_helper
