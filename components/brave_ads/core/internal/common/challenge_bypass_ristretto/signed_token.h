/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "base/check.h"
#include "brave/components/challenge_bypass_ristretto/signed_token.h"

namespace brave_ads::cbr {

// A `SignedToken` is the result of signing a `BlindedToken`.

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
  std::optional<std::string> EncodeBase64() const;

 private:
  std::optional<challenge_bypass_ristretto::SignedToken> signed_token_;
};

std::ostream& operator<<(std::ostream& os, const SignedToken& signed_token);

using SignedTokenList = std::vector<SignedToken>;

}  // namespace brave_ads::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_H_
