/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/token.h"

#include <utility>

#include "brave/components/challenge_bypass_ristretto/blinded_token.h"

namespace challenge_bypass_ristretto {

Token::Token(CxxTokenBox raw)
    : raw_(base::MakeRefCounted<CxxTokenRefData>(
          CxxTokenValueOrResult(std::move(raw)))) {}

Token::Token(CxxTokenResultBox raw)
    : raw_(base::MakeRefCounted<CxxTokenRefData>(
          CxxTokenValueOrResult(std::move(raw)))) {}

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

BlindedToken Token::Blind() {
  rust::Box<cbr_cxx::BlindedToken> raw_blinded_token(raw().blind());
  return BlindedToken(std::move(raw_blinded_token));
}

// static
base::expected<Token, std::string> Token::DecodeBase64(
    const std::string& encoded) {
  CxxTokenResultBox raw_token_result(cbr_cxx::decode_base64_token(encoded));

  if (!raw_token_result->is_ok()) {
    return base::unexpected("Failed to decode token");
  }

  return Token(std::move(raw_token_result));
}

std::string Token::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool Token::operator==(const Token& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
