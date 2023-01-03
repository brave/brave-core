/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_

#include <ostream>
#include <string>

#include "absl/types/optional.h"
#include "base/check.h"
#include "wrapper.hpp"

namespace ads::privacy::cbr {

// A |BlindedToken| is sent to the server for signing. It is the result of the
// scalar multiplication of the point derived from the |TokenPreimage| with the
// blinding factor. (P = T^r = H_1(t)^r).

class BlindedToken {
 public:
  BlindedToken();
  explicit BlindedToken(const std::string& blinded_token_base64);
  explicit BlindedToken(
      const challenge_bypass_ristretto::BlindedToken& blinded_token);

  BlindedToken(const BlindedToken& other);
  BlindedToken& operator=(const BlindedToken& other);

  BlindedToken(BlindedToken&& other) noexcept;
  BlindedToken& operator=(BlindedToken&& other) noexcept;

  ~BlindedToken();

  bool operator==(const BlindedToken& other) const;
  bool operator!=(const BlindedToken& other) const;

  bool has_value() const {
    return blinded_token_ && blinded_token_.has_value();
  }

  challenge_bypass_ristretto::BlindedToken& get() {
    DCHECK(blinded_token_);
    return *blinded_token_;
  }

  const challenge_bypass_ristretto::BlindedToken& get() const {
    DCHECK(blinded_token_);
    return *blinded_token_;
  }

  static BlindedToken DecodeBase64(const std::string& blinded_token_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::BlindedToken> blinded_token_;
};

std::ostream& operator<<(std::ostream& os, const BlindedToken& blinded_token);

}  // namespace ads::privacy::cbr

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_
