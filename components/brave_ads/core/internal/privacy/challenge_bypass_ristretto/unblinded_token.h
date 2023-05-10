/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/wrapper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy::cbr {

class TokenPreimage;
class VerificationKey;

// An |UnblindedToken| is the result of unblinding a |SignedToken|. While both
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
  absl::optional<std::string> EncodeBase64() const;

  absl::optional<VerificationKey> DeriveVerificationKey() const;

  absl::optional<TokenPreimage> GetTokenPreimage() const;

 private:
  absl::optional<challenge_bypass_ristretto::UnblindedToken> unblinded_token_;
};

std::ostream& operator<<(std::ostream& os,
                         const UnblindedToken& unblinded_token);

}  // namespace brave_ads::privacy::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_UNBLINDED_TOKEN_H_
