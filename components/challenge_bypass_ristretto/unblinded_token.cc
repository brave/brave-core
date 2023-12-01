/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/unblinded_token.h"

#include <utility>

#include "brave/components/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/challenge_bypass_ristretto/verification_key.h"

namespace challenge_bypass_ristretto {

UnblindedToken::UnblindedToken(CxxUnblindedTokenBox raw)
    : raw_(base::MakeRefCounted<CxxUnblindedTokenRefData>(
          CxxUnblindedTokenValueOrResult(std::move(raw)))) {}

UnblindedToken::UnblindedToken(CxxUnblindedTokenResultBox raw)
    : raw_(base::MakeRefCounted<CxxUnblindedTokenRefData>(
          CxxUnblindedTokenValueOrResult(std::move(raw)))) {}

UnblindedToken::UnblindedToken(const UnblindedToken& other) = default;

UnblindedToken& UnblindedToken::operator=(const UnblindedToken& other) =
    default;

UnblindedToken::UnblindedToken(UnblindedToken&& other) noexcept = default;

UnblindedToken& UnblindedToken::operator=(UnblindedToken&& other) noexcept =
    default;

UnblindedToken::~UnblindedToken() = default;

VerificationKey UnblindedToken::DeriveVerificationKey() const {
  return VerificationKey(raw().derive_verification_key());
}

TokenPreimage UnblindedToken::Preimage() const {
  return TokenPreimage(raw().preimage());
}

// static
base::expected<UnblindedToken, std::string> UnblindedToken::DecodeBase64(
    const std::string& encoded) {
  CxxUnblindedTokenResultBox raw_unblinded_token_result(
      cbr_cxx::decode_base64_unblinded_token(encoded));

  if (!raw_unblinded_token_result->is_ok()) {
    return base::unexpected("Failed to decode unblinded token");
  }

  return UnblindedToken(std::move(raw_unblinded_token_result));
}

std::string UnblindedToken::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool UnblindedToken::operator==(const UnblindedToken& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
