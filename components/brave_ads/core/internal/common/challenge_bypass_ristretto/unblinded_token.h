/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_

#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "base/check.h"
#include "brave/components/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads::cbr {

class TokenPreimage;
class VerificationKey;

// An `UnblindedToken` is the result of unblinding a `SignedToken`. While both
// the client and server both "know" this value, it should nevertheless not be
// sent between the two.

class UnblindedToken {
 public:
  UnblindedToken();
  explicit UnblindedToken(const std::string& unblinded_token_base64);
  explicit UnblindedToken(
      const challenge_bypass_ristretto::UnblindedToken& unblinded_token);

  UnblindedToken(const UnblindedToken&);
  UnblindedToken& operator=(const UnblindedToken&);

  UnblindedToken(UnblindedToken&&) noexcept;
  UnblindedToken& operator=(UnblindedToken&&) noexcept;

  ~UnblindedToken();

  bool operator==(const UnblindedToken&) const;
  bool operator!=(const UnblindedToken&) const;

  bool has_value() const {
    return unblinded_token_ && unblinded_token_.has_value();
  }

  challenge_bypass_ristretto::UnblindedToken& get() {
    CHECK(unblinded_token_);
    return *unblinded_token_;
  }

  const challenge_bypass_ristretto::UnblindedToken& get() const {
    CHECK(unblinded_token_);
    return *unblinded_token_;
  }

  static UnblindedToken DecodeBase64(const std::string& unblinded_token_base64);
  std::optional<std::string> EncodeBase64() const;

  std::optional<VerificationKey> DeriveVerificationKey() const;

  std::optional<TokenPreimage> GetTokenPreimage() const;

 private:
  std::optional<challenge_bypass_ristretto::UnblindedToken> unblinded_token_;
};

std::ostream& operator<<(std::ostream& os,
                         const UnblindedToken& unblinded_token);

using UnblindedTokenList = std::vector<UnblindedToken>;

}  // namespace brave_ads::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_
