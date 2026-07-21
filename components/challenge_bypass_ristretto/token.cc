/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/token.h"

#include <utility>

#include "brave/components/challenge_bypass_ristretto/blinded_token.h"

namespace challenge_bypass_ristretto {

Token::Token(CxxTokenBox raw)
    : raw_(base::MakeRefCounted<CxxTokenRefData>(std::move(raw))) {}

Token::Token(const Token& other) = default;

Token& Token::operator=(const Token& other) = default;

Token::Token(Token&& other) noexcept = default;

Token& Token::operator=(Token&& other) noexcept = default;

Token::~Token() = default;

// static
Token Token::Random() {
  CxxTokenBox raw_token(cbr_cxx::generate_token());
  return Token(std::move(raw_token));
}

base::expected<BlindedToken, std::string> Token::Blind() {
  rust::Box<cbr_cxx::BlindedTokenResult> raw_blinded_token_result(
      raw().blind());

  if (!raw_blinded_token_result->is_ok()) {
    return base::unexpected("Failed to blind token");
  }

  return BlindedToken(raw_blinded_token_result->unwrap());
}

// static
base::expected<Token, std::string> Token::DecodeBase64(
    const std::string& encoded) {
  rust::Box<cbr_cxx::TokenResult> raw_token_result(
      cbr_cxx::decode_base64_token(encoded));

  if (!raw_token_result->is_ok()) {
    return base::unexpected("Failed to decode token");
  }

  return Token(raw_token_result->unwrap());
}

std::string Token::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool Token::operator==(const Token& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
