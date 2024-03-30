/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/signed_token.h"

#include <utility>

namespace challenge_bypass_ristretto {

SignedToken::SignedToken(CxxSignedTokenBox raw)
    : raw_(base::MakeRefCounted<CxxSignedTokenRefData>(std::move(raw))) {}

SignedToken::SignedToken(const SignedToken&) = default;

SignedToken& SignedToken::operator=(const SignedToken&) = default;

SignedToken::SignedToken(SignedToken&&) noexcept = default;

SignedToken& SignedToken::operator=(SignedToken&&) noexcept = default;

SignedToken::~SignedToken() = default;

base::expected<SignedToken, std::string> SignedToken::DecodeBase64(
    const std::string& encoded) {
  rust::Box<cbr_cxx::SignedTokenResult> raw_signed_token_result(
      cbr_cxx::decode_base64_signed_token(encoded));

  if (!raw_signed_token_result->is_ok()) {
    return base::unexpected("Failed to decode signed token");
  }

  return SignedToken(raw_signed_token_result->unwrap());
}

std::string SignedToken::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool SignedToken::operator==(const SignedToken& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
