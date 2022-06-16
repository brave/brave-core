/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_TOKEN_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_TOKEN_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

class BlindedToken;

// A |Token| consists of a randomly chosen preimage and blinding factor. Since a
// |Token| includes the blinding factor it should be treated as a client secret
// and NEVER revealed to the server.

class Token {
 public:
  Token();
  explicit Token(const std::string& token_base64);
  Token(const Token& other);
  Token& operator=(const Token& other);
  ~Token();

  bool operator==(const Token& rhs) const;
  bool operator!=(const Token& rhs) const;

  bool has_value() const { return token_.has_value(); }

  challenge_bypass_ristretto::Token get() const {
    DCHECK(has_value());
    return token_.value();
  }

  static Token DecodeBase64(const std::string& token_base64);
  absl::optional<std::string> EncodeBase64() const;

  absl::optional<BlindedToken> Blind();

 private:
  absl::optional<challenge_bypass_ristretto::Token> token_;
};

std::ostream& operator<<(std::ostream& os, const Token& token);

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_TOKEN_H_
