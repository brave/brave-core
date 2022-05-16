/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

namespace ads {
namespace privacy {
namespace cbr {

namespace {

absl::optional<challenge_bypass_ristretto::PublicKey> Create(
    const std::string& public_key_base64) {
  if (public_key_base64.empty()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::PublicKey raw_public_key =
      challenge_bypass_ristretto::PublicKey::decode_base64(public_key_base64);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_public_key;
}

}  // namespace

PublicKey::PublicKey() = default;

PublicKey::PublicKey(const std::string& public_key_base64)
    : public_key_(Create(public_key_base64)) {}

PublicKey::PublicKey(const challenge_bypass_ristretto::PublicKey& public_key)
    : public_key_(public_key) {}

PublicKey::PublicKey(const PublicKey& other) = default;

PublicKey::~PublicKey() = default;

bool PublicKey::operator==(const PublicKey& rhs) const {
  return EncodeBase64().value_or("") == rhs.EncodeBase64().value_or("");
}

bool PublicKey::operator!=(const PublicKey& rhs) const {
  return !(*this == rhs);
}

PublicKey PublicKey::DecodeBase64(const std::string& public_key_base64) {
  return PublicKey(public_key_base64);
}

absl::optional<std::string> PublicKey::EncodeBase64() const {
  if (!has_value()) {
    return absl::nullopt;
  }

  const std::string encoded_base64 = public_key_->encode_base64();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return encoded_base64;
}

std::ostream& operator<<(std::ostream& os, const PublicKey& public_key) {
  os << public_key.EncodeBase64().value_or("");
  return os;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
