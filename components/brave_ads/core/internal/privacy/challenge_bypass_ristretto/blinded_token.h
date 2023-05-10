/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/wrapper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy::cbr {

// A |BlindedToken| is sent to the server for signing. It is the result of the
// scalar multiplication of the point derived from the |TokenPreimage| with the
// blinding factor. (P = T^r = H_1(t)^r).

class BlindedToken {
 public:
  BlindedToken();
  explicit BlindedToken(const std::string& blinded_token_base64);
  explicit BlindedToken(
      const challenge_bypass_ristretto::BlindedToken& blinded_token);

  BlindedToken(const BlindedToken&);
  BlindedToken& operator=(const BlindedToken&);

  BlindedToken(BlindedToken&&) noexcept;
  BlindedToken& operator=(BlindedToken&&) noexcept;

  ~BlindedToken();

  bool operator==(const BlindedToken&) const;
  bool operator!=(const BlindedToken&) const;

  bool has_value() const {
    return blinded_token_ && blinded_token_.has_value();
  }

  challenge_bypass_ristretto::BlindedToken& get() {
    CHECK(blinded_token_);
    return *blinded_token_;
  }

  const challenge_bypass_ristretto::BlindedToken& get() const {
    CHECK(blinded_token_);
    return *blinded_token_;
  }

  static BlindedToken DecodeBase64(const std::string& blinded_token_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::BlindedToken> blinded_token_;
};

std::ostream& operator<<(std::ostream& os, const BlindedToken& blinded_token);

}  // namespace brave_ads::privacy::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_H_
