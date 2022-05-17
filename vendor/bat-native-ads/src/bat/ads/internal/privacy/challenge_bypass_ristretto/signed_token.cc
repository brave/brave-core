/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

namespace ads {
namespace privacy {
namespace cbr {

namespace {

absl::optional<challenge_bypass_ristretto::SignedToken> Create(
    const std::string& signed_token_base64) {
  if (signed_token_base64.empty()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::SignedToken raw_signed_token =
      challenge_bypass_ristretto::SignedToken::decode_base64(
          signed_token_base64);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_signed_token;
}

}  // namespace

SignedToken::SignedToken() = default;

SignedToken::SignedToken(const std::string& signed_token_base64)
    : signed_token_(Create(signed_token_base64)) {}

SignedToken::SignedToken(
    const challenge_bypass_ristretto::SignedToken& signed_token)
    : signed_token_(signed_token) {}

SignedToken::SignedToken(const SignedToken& other) = default;

SignedToken::~SignedToken() = default;

bool SignedToken::operator==(const SignedToken& rhs) const {
  return EncodeBase64().value_or("") == rhs.EncodeBase64().value_or("");
}

bool SignedToken::operator!=(const SignedToken& rhs) const {
  return !(*this == rhs);
}

SignedToken SignedToken::DecodeBase64(const std::string& signed_token_base64) {
  return SignedToken(signed_token_base64);
}

absl::optional<std::string> SignedToken::EncodeBase64() const {
  if (!has_value()) {
    return absl::nullopt;
  }

  const std::string encoded_base64 = signed_token_->encode_base64();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return encoded_base64;
}

std::ostream& operator<<(std::ostream& os, const SignedToken& signed_token) {
  os << signed_token.EncodeBase64().value_or("");
  return os;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
