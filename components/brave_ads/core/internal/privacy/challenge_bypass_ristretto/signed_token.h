/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/wrapper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy::cbr {

// A |SignedToken| is the result of signing a |BlindedToken|.

class SignedToken {
 public:
  SignedToken();
  explicit SignedToken(const std::string& signed_token_base64);
  explicit SignedToken(
      const challenge_bypass_ristretto::SignedToken& signed_token);

  SignedToken(const SignedToken&);
  SignedToken& operator=(const SignedToken&);

  SignedToken(SignedToken&&) noexcept;
  SignedToken& operator=(SignedToken&&) noexcept;

  ~SignedToken();

  bool operator==(const SignedToken&) const;
  bool operator!=(const SignedToken&) const;

  bool has_value() const { return signed_token_.has_value(); }

  challenge_bypass_ristretto::SignedToken& get() {
    CHECK(signed_token_);
    return *signed_token_;
  }

  const challenge_bypass_ristretto::SignedToken& get() const {
    CHECK(signed_token_);
    return *signed_token_;
  }

  static SignedToken DecodeBase64(const std::string& signed_token_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::SignedToken> signed_token_;
};

std::ostream& operator<<(std::ostream& os, const SignedToken& signed_token);

}  // namespace brave_ads::privacy::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_
