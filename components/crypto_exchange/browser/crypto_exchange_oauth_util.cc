/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/crypto_exchange/browser/crypto_exchange_oauth_util.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "crypto/random.h"
#include "crypto/sha2.h"

namespace crypto_exchange {

std::string GetCryptoRandomString(bool hex_encode) {
  const size_t kSeedByteLength = 32;
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes, kSeedByteLength);

  if (!hex_encode) {
    std::string encoded_string;
    base::Base64Encode(
      reinterpret_cast<char*>(random_seed_bytes), &encoded_string);
    return encoded_string;
  }

  return base::HexEncode(
      reinterpret_cast<char*>(random_seed_bytes), kSeedByteLength);
}

std::string GetCodeChallenge(
    const std::string& code_verifier, bool strip_chars) {
  std::string code_challenge;
  char raw[crypto::kSHA256Length] = {0};
  crypto::SHA256HashString(code_verifier,
                           raw,
                           crypto::kSHA256Length);
  base::Base64Encode(base::StringPiece(raw,
                                       crypto::kSHA256Length),
                                       &code_challenge);

  if (strip_chars) {
    std::replace(code_challenge.begin(), code_challenge.end(), '+', '-');
    std::replace(code_challenge.begin(), code_challenge.end(), '/', '_');

    code_challenge.erase(std::find_if(code_challenge.rbegin(),
        code_challenge.rend(), [](int ch) {
      return ch != '=';
    }).base(), code_challenge.end());
  }

  return code_challenge;
}

}  // namespace crypto_exchange
