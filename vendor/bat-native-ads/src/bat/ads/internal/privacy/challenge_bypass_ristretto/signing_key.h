/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

class BlindedToken;
class PublicKey;
class SignedToken;
class TokenPreimage;
class UnblindedToken;

// A |SigningKey| is used to sign a |BlindedToken| and to verify an
// |UnblindedToken|.

class SigningKey {
 public:
  SigningKey();
  explicit SigningKey(const std::string& signing_key_base64);
  explicit SigningKey(
      const challenge_bypass_ristretto::SigningKey& signing_key);
  SigningKey(const SigningKey& other);
  ~SigningKey();

  bool operator==(const SigningKey& rhs) const;
  bool operator!=(const SigningKey& rhs) const;

  bool has_value() const { return signing_key_.has_value(); }

  challenge_bypass_ristretto::SigningKey get() const {
    DCHECK(has_value());
    return signing_key_.value();
  }

  static SigningKey DecodeBase64(const std::string& signing_key_base64);
  absl::optional<std::string> EncodeBase64() const;

  absl::optional<SignedToken> Sign(const BlindedToken& blinded_token) const;

  absl::optional<UnblindedToken> RederiveUnblindedToken(
      const TokenPreimage& token_preimage);

  absl::optional<PublicKey> GetPublicKey();

 private:
  absl::optional<challenge_bypass_ristretto::SigningKey> signing_key_;
};

std::ostream& operator<<(std::ostream& os, const SigningKey& signing_key);

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_SIGNING_KEY_H_
