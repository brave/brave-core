/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_

#include <optional>
#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/components/challenge_bypass_ristretto/signing_key.h"

namespace brave_ads::cbr {

class BlindedToken;
class PublicKey;
class SignedToken;
class TokenPreimage;
class UnblindedToken;

// A `SigningKey` is used to sign a `BlindedToken` and to verify an
// `UnblindedToken`.

class SigningKey {
 public:
  SigningKey();
  explicit SigningKey(const std::string& signing_key_base64);
  explicit SigningKey(
      const challenge_bypass_ristretto::SigningKey& signing_key);

  SigningKey(const SigningKey&) = delete;
  SigningKey& operator=(const SigningKey&) = delete;

  ~SigningKey();

  bool operator==(const SigningKey&) const;
  bool operator!=(const SigningKey&) const;

  bool has_value() const { return signing_key_.has_value(); }

  challenge_bypass_ristretto::SigningKey& get() {
    CHECK(signing_key_);
    return *signing_key_;
  }

  const challenge_bypass_ristretto::SigningKey& get() const {
    CHECK(signing_key_);
    return *signing_key_;
  }

  static SigningKey DecodeBase64(const std::string& signing_key_base64);
  std::optional<std::string> EncodeBase64() const;

  std::optional<SignedToken> Sign(const BlindedToken& blinded_token) const;

  std::optional<UnblindedToken> RederiveUnblindedToken(
      const TokenPreimage& token_preimage);

  std::optional<PublicKey> GetPublicKey();

 private:
  std::optional<challenge_bypass_ristretto::SigningKey> signing_key_;
};

std::ostream& operator<<(std::ostream& os, const SigningKey& signing_key);

}  // namespace brave_ads::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_
