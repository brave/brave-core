/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/token_preimage.h"

#include <utility>

namespace challenge_bypass_ristretto {

TokenPreimage::TokenPreimage(CxxTokenPreimageBox raw)
    : raw_(base::MakeRefCounted<CxxTokenPreimageRefData>(std::move(raw))) {}

TokenPreimage::TokenPreimage(const TokenPreimage&) = default;

TokenPreimage& TokenPreimage::operator=(const TokenPreimage&) = default;

TokenPreimage::TokenPreimage(TokenPreimage&&) noexcept = default;

TokenPreimage& TokenPreimage::operator=(TokenPreimage&&) noexcept = default;

TokenPreimage::~TokenPreimage() = default;

// static
base::expected<TokenPreimage, std::string> TokenPreimage::DecodeBase64(
    const std::string& encoded) {
  rust::Box<cbr_cxx::TokenPreimageResult> raw_token_preimage_result(
      cbr_cxx::decode_base64_token_preimage(encoded));

  if (!raw_token_preimage_result->is_ok()) {
    return base::unexpected("Failed to decode token preimage");
  }

  return TokenPreimage(raw_token_preimage_result->unwrap());
}

std::string TokenPreimage::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool TokenPreimage::operator==(const TokenPreimage& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
