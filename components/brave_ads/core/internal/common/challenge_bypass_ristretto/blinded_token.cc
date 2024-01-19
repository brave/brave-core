/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

namespace brave_ads::cbr {

namespace {

std::optional<challenge_bypass_ristretto::BlindedToken> Create(
    const std::string& blinded_token_base64) {
  if (blinded_token_base64.empty()) {
    return std::nullopt;
  }

  return ValueOrLogError(challenge_bypass_ristretto::BlindedToken::DecodeBase64(
      blinded_token_base64));
}

}  // namespace

BlindedToken::BlindedToken() = default;

BlindedToken::BlindedToken(const std::string& blinded_token_base64)
    : blinded_token_(Create(blinded_token_base64)) {}

BlindedToken::BlindedToken(
    const challenge_bypass_ristretto::BlindedToken& blinded_token)
    : blinded_token_(blinded_token) {}

BlindedToken::BlindedToken(const BlindedToken& other) = default;

BlindedToken& BlindedToken::operator=(const BlindedToken& other) = default;

BlindedToken::BlindedToken(BlindedToken&& other) noexcept = default;

BlindedToken& BlindedToken::operator=(BlindedToken&& other) noexcept = default;

BlindedToken::~BlindedToken() = default;

bool BlindedToken::operator==(const BlindedToken& other) const {
  return EncodeBase64().value_or("") == other.EncodeBase64().value_or("");
}

bool BlindedToken::operator!=(const BlindedToken& other) const {
  return !(*this == other);
}

BlindedToken BlindedToken::DecodeBase64(
    const std::string& blinded_token_base64) {
  return BlindedToken(blinded_token_base64);
}

std::optional<std::string> BlindedToken::EncodeBase64() const {
  if (!blinded_token_ || !has_value()) {
    return std::nullopt;
  }

  return blinded_token_->EncodeBase64();
}

std::ostream& operator<<(std::ostream& os, const BlindedToken& blinded_token) {
  os << blinded_token.EncodeBase64().value_or("");
  return os;
}

}  // namespace brave_ads::cbr
