/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_

#include <optional>
#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/components/challenge_bypass_ristretto/token_preimage.h"

namespace brave_ads::cbr {

// A `TokenPreimage` is a slice of bytes which can be hashed to a ristretto
// point. The hash function must ensure the discrete log with respect to other
// points is unknown.

class TokenPreimage {
 public:
  TokenPreimage();
  explicit TokenPreimage(const std::string& token_preimage_base64);
  explicit TokenPreimage(
      const challenge_bypass_ristretto::TokenPreimage& token_preimage);

  TokenPreimage(const TokenPreimage&);
  TokenPreimage& operator=(const TokenPreimage&);

  TokenPreimage(TokenPreimage&&) noexcept;
  TokenPreimage& operator=(TokenPreimage&&) noexcept;

  ~TokenPreimage();

  bool operator==(const TokenPreimage&) const;
  bool operator!=(const TokenPreimage&) const;

  bool has_value() const {
    return token_preimage_ && token_preimage_.has_value();
  }

  challenge_bypass_ristretto::TokenPreimage& get() {
    CHECK(token_preimage_);
    return *token_preimage_;
  }

  const challenge_bypass_ristretto::TokenPreimage& get() const {
    CHECK(token_preimage_);
    return *token_preimage_;
  }

  static TokenPreimage DecodeBase64(const std::string& token_preimage_base64);
  std::optional<std::string> EncodeBase64() const;

 private:
  std::optional<challenge_bypass_ristretto::TokenPreimage> token_preimage_;
};

std::ostream& operator<<(std::ostream& os, const TokenPreimage& token_preimage);

}  // namespace brave_ads::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_
