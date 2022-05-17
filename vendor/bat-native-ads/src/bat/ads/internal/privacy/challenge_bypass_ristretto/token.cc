/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

namespace ads {
namespace privacy {
namespace cbr {

namespace {

absl::optional<challenge_bypass_ristretto::Token> Create() {
  const challenge_bypass_ristretto::Token raw_token =
      challenge_bypass_ristretto::Token::random();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_token;
}

absl::optional<challenge_bypass_ristretto::Token> Create(
    const std::string& token_base64) {
  if (token_base64.empty()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::Token raw_token =
      challenge_bypass_ristretto::Token::decode_base64(token_base64);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_token;
}

}  // namespace

Token::Token() : token_(Create()) {}

Token::Token(const std::string& token_base64) : token_(Create(token_base64)) {}

Token::Token(const Token& other) = default;

Token::~Token() = default;

bool Token::operator==(const Token& rhs) const {
  return EncodeBase64().value_or("") == rhs.EncodeBase64().value_or("");
}

bool Token::operator!=(const Token& rhs) const {
  return !(*this == rhs);
}

Token Token::DecodeBase64(const std::string& token_base64) {
  return Token(token_base64);
}

absl::optional<std::string> Token::EncodeBase64() const {
  if (!has_value()) {
    return absl::nullopt;
  }

  const std::string encoded_base64 = token_->encode_base64();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return encoded_base64;
}

absl::optional<BlindedToken> Token::Blind() {
  if (!has_value()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::BlindedToken raw_blinded_token =
      token_->blind();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return BlindedToken(raw_blinded_token);
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
  os << token.EncodeBase64().value_or("");
  return os;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
