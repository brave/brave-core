/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

namespace ads {
namespace privacy {
namespace cbr {

namespace {

absl::optional<challenge_bypass_ristretto::BlindedToken> Create(
    const std::string& blinded_token_base64) {
  if (blinded_token_base64.empty()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::BlindedToken raw_blinded_token =
      challenge_bypass_ristretto::BlindedToken::decode_base64(
          blinded_token_base64);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_blinded_token;
}

}  // namespace

BlindedToken::BlindedToken() = default;

BlindedToken::BlindedToken(const std::string& blinded_token_base64)
    : blinded_token_(Create(blinded_token_base64)) {}

BlindedToken::BlindedToken(
    const challenge_bypass_ristretto::BlindedToken& blinded_token)
    : blinded_token_(blinded_token) {}

BlindedToken::BlindedToken(const BlindedToken& other) = default;

BlindedToken::~BlindedToken() = default;

bool BlindedToken::operator==(const BlindedToken& rhs) const {
  return EncodeBase64().value_or("") == rhs.EncodeBase64().value_or("");
}

bool BlindedToken::operator!=(const BlindedToken& rhs) const {
  return !(*this == rhs);
}

BlindedToken BlindedToken::DecodeBase64(
    const std::string& blinded_token_base64) {
  return BlindedToken(blinded_token_base64);
}

absl::optional<std::string> BlindedToken::EncodeBase64() const {
  if (!has_value()) {
    return absl::nullopt;
  }

  const std::string encoded_base64 = blinded_token_->encode_base64();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return encoded_base64;
}

std::ostream& operator<<(std::ostream& os, const BlindedToken& blinded_token) {
  os << blinded_token.EncodeBase64().value_or("");
  return os;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
