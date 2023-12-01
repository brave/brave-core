/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/blinded_token.h"

#include <utility>

namespace challenge_bypass_ristretto {

BlindedToken::BlindedToken(CxxBlindedTokenBox raw)
    : raw_(base::MakeRefCounted<CxxBlindedTokenData>(
          CxxBlindedTokenValueOrResult(std::move(raw)))) {}

BlindedToken::BlindedToken(CxxBlindedTokenResultBox raw)
    : raw_(base::MakeRefCounted<CxxBlindedTokenData>(
          CxxBlindedTokenValueOrResult(std::move(raw)))) {}

BlindedToken::BlindedToken(const BlindedToken& other) = default;

BlindedToken& BlindedToken::operator=(const BlindedToken& other) = default;

BlindedToken::BlindedToken(BlindedToken&& other) noexcept = default;

BlindedToken& BlindedToken::operator=(BlindedToken&& other) noexcept = default;

BlindedToken::~BlindedToken() = default;

base::expected<BlindedToken, std::string> BlindedToken::DecodeBase64(
    const std::string& encoded) {
  rust::Box<cbr_cxx::BlindedTokenResult> blinded_token_result(
      cbr_cxx::decode_base64_blinded_token(encoded));

  if (!blinded_token_result->is_ok()) {
    return base::unexpected("Failed to decode blinded token");
  }

  return BlindedToken(std::move(blinded_token_result));
}

std::string BlindedToken::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool BlindedToken::operator==(const BlindedToken& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
