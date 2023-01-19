/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_

#include <ostream>
#include <string>

#include "absl/types/optional.h"
#include "base/check.h"
#include "wrapper.hpp"

namespace ads::privacy::cbr {

// A |PublicKey| is a commitment by the server to a particular |SigningKey|.

class PublicKey {
 public:
  PublicKey();
  explicit PublicKey(const std::string& public_key_base64);
  explicit PublicKey(const challenge_bypass_ristretto::PublicKey& public_key);

  PublicKey(const PublicKey& other);
  PublicKey& operator=(const PublicKey& other);

  PublicKey(PublicKey&& other) noexcept;
  PublicKey& operator=(PublicKey&& other) noexcept;

  ~PublicKey();

  bool operator==(const PublicKey& other) const;
  bool operator!=(const PublicKey& other) const;

  bool has_value() const { return public_key_.has_value(); }

  challenge_bypass_ristretto::PublicKey& get() {
    DCHECK(public_key_);
    return *public_key_;
  }

  const challenge_bypass_ristretto::PublicKey& get() const {
    DCHECK(public_key_);
    return *public_key_;
  }

  static PublicKey DecodeBase64(const std::string& public_key_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::PublicKey> public_key_;
};

std::ostream& operator<<(std::ostream& os, const PublicKey& public_key);

}  // namespace ads::privacy::cbr

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
