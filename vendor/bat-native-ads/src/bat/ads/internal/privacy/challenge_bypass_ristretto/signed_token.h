/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

// A |SignedToken| is the result of signing a |BlindedToken|.

class SignedToken {
 public:
  SignedToken();
  explicit SignedToken(const std::string& signed_token_base64);
  explicit SignedToken(
      const challenge_bypass_ristretto::SignedToken& signed_token);
  SignedToken(const SignedToken& other);
  ~SignedToken();

  bool operator==(const SignedToken& rhs) const;
  bool operator!=(const SignedToken& rhs) const;

  bool has_value() const { return signed_token_.has_value(); }

  challenge_bypass_ristretto::SignedToken get() const {
    DCHECK(has_value());
    return signed_token_.value();
  }

  static SignedToken DecodeBase64(const std::string& signed_token_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::SignedToken> signed_token_;
};

std::ostream& operator<<(std::ostream& os, const SignedToken& signed_token);

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_
