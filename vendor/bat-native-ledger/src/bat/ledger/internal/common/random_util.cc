/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/common/random_util.h"

#include "base/base64url.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "bat/ledger/ledger.h"
#include "crypto/random.h"
#include "crypto/sha2.h"

namespace {

std::string EncodeStringForPKCE(const std::string& data) {
  std::string encoded_data;

  base::Base64UrlEncode(data, base::Base64UrlEncodePolicy::OMIT_PADDING,
                        &encoded_data);
  base::ReplaceChars(encoded_data, "+", "-", &encoded_data);
  base::ReplaceChars(encoded_data, "/", "_", &encoded_data);

  return encoded_data;
}

}  // namespace

namespace ledger {
namespace util {

std::string GenerateRandomHexString() {
  if (ledger::is_testing) {
    return "123456789";
  }

  const size_t kLength = 32;
  uint8_t bytes[kLength];
  crypto::RandBytes(bytes, sizeof(bytes));
  return base::HexEncode(bytes, sizeof(bytes));
}

std::string GeneratePKCECodeVerifier() {
  return EncodeStringForPKCE(GenerateRandomHexString());
}

std::string GeneratePKCECodeChallenge(const std::string& code_verifier) {
  return EncodeStringForPKCE(crypto::SHA256HashString(code_verifier));
}

}  // namespace util
}  // namespace ledger
