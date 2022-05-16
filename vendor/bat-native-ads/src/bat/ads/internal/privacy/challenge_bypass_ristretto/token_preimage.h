/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

// A |TokenPreimage| is a slice of bytes which can be hashed to a ristretto
// point. The hash function must ensure the discrete log with respect to other
// points is unknown.

class TokenPreimage {
 public:
  TokenPreimage();
  explicit TokenPreimage(const std::string& token_preimage_base64);
  explicit TokenPreimage(
      const challenge_bypass_ristretto::TokenPreimage& token_preimage);
  TokenPreimage(const TokenPreimage& other);
  ~TokenPreimage();

  bool operator==(const TokenPreimage& rhs) const;
  bool operator!=(const TokenPreimage& rhs) const;

  bool has_value() const { return token_preimage_.has_value(); }

  challenge_bypass_ristretto::TokenPreimage get() const {
    DCHECK(has_value());
    return token_preimage_.value();
  }

  static TokenPreimage DecodeBase64(const std::string& token_preimage_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::TokenPreimage> token_preimage_;
};

std::ostream& operator<<(std::ostream& os, const TokenPreimage& token_preimage);

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_TOKEN_PREIMAGE_H_
