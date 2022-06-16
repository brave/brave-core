/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

// A |PublicKey| is a commitment by the server to a particular |SigningKey|.

class PublicKey {
 public:
  PublicKey();
  explicit PublicKey(const std::string& public_key_base64);
  explicit PublicKey(const challenge_bypass_ristretto::PublicKey& public_key);
  PublicKey(const PublicKey& other);
  PublicKey& operator=(const PublicKey& other);
  ~PublicKey();

  bool operator==(const PublicKey& rhs) const;
  bool operator!=(const PublicKey& rhs) const;

  bool has_value() const { return public_key_.has_value(); }

  challenge_bypass_ristretto::PublicKey get() const {
    DCHECK(has_value());
    return public_key_.value();
  }

  static PublicKey DecodeBase64(const std::string& public_key_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::PublicKey> public_key_;
};

std::ostream& operator<<(std::ostream& os, const PublicKey& public_key);

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
