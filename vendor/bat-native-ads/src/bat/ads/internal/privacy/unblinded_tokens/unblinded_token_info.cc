/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

#include <string>

#include "bat/ads/internal/logging.h"

namespace ads {
namespace privacy {

UnblindedTokenInfo::UnblindedTokenInfo()
    : value(nullptr),
      public_key(nullptr) {}

UnblindedTokenInfo::UnblindedTokenInfo(
    const UnblindedTokenInfo& unblinded_token) = default;

UnblindedTokenInfo::~UnblindedTokenInfo() = default;

bool UnblindedTokenInfo::operator==(
    const UnblindedTokenInfo& rhs) const {

  const std::string public_key_base64 = public_key.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  const std::string rhs_public_key_base64 = rhs.public_key.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  const std::string value_base64 = value.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  const std::string rhs_value_base64 = rhs.value.encode_base64();
  if (challenge_bypass_ristretto::exception_occurred()) {
    challenge_bypass_ristretto::TokenException e =
        challenge_bypass_ristretto::get_last_exception();
    BLOG(0, "Challenge Bypass Ristretto Error: " << e.what());
    return false;
  }

  return public_key_base64 == rhs_public_key_base64 &&
      value_base64 == rhs_value_base64;
}

bool UnblindedTokenInfo::operator!=(
    const UnblindedTokenInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace privacy
}  // namespace ads
