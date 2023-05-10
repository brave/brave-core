/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/wrapper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy::cbr {

// A |PublicKey| is a commitment by the server to a particular |SigningKey|.

class PublicKey {
 public:
  PublicKey();
  explicit PublicKey(const std::string& public_key_base64);
  explicit PublicKey(const challenge_bypass_ristretto::PublicKey& public_key);

  PublicKey(const PublicKey&);
  PublicKey& operator=(const PublicKey&);

  PublicKey(PublicKey&&) noexcept;
  PublicKey& operator=(PublicKey&&) noexcept;

  ~PublicKey();

  bool operator==(const PublicKey&) const;
  bool operator!=(const PublicKey&) const;

  bool has_value() const { return public_key_.has_value(); }

  challenge_bypass_ristretto::PublicKey& get() {
    CHECK(public_key_);
    return *public_key_;
  }

  const challenge_bypass_ristretto::PublicKey& get() const {
    CHECK(public_key_);
    return *public_key_;
  }

  static PublicKey DecodeBase64(const std::string& public_key_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::PublicKey> public_key_;
};

std::ostream& operator<<(std::ostream& os, const PublicKey& public_key);

}  // namespace brave_ads::privacy::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_PUBLIC_KEY_H_
