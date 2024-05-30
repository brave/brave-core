/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_widget_utils/browser/ntp_widget_utils_oauth.h"

#include <string>
#include <string_view>

#include "base/base64.h"
#include "base/containers/adapters.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "crypto/random.h"
#include "crypto/sha2.h"

namespace ntp_widget_utils {

std::string GetCryptoRandomString(bool hex_encode) {
  constexpr size_t kSeedByteLength = 32;
  uint8_t random_seed_bytes[kSeedByteLength];
  crypto::RandBytes(random_seed_bytes);

  if (!hex_encode) {
    return base::Base64Encode(random_seed_bytes);
  }

  return base::HexEncode(random_seed_bytes, kSeedByteLength);
}

std::string GetCodeChallenge(
    const std::string& code_verifier, bool strip_chars) {
  char raw[crypto::kSHA256Length] = {0};
  crypto::SHA256HashString(code_verifier,
                           raw,
                           crypto::kSHA256Length);
  std::string code_challenge =
      base::Base64Encode(std::string_view(raw, crypto::kSHA256Length));

  if (strip_chars) {
    std::replace(code_challenge.begin(), code_challenge.end(), '+', '-');
    std::replace(code_challenge.begin(), code_challenge.end(), '/', '_');

    code_challenge.erase(base::ranges::find_if(base::Reversed(code_challenge),
                                               [](int ch) { return ch != '='; })
                             .base(),
                         code_challenge.end());
  }

  return code_challenge;
}

}  // namespace ntp_widget_utils
