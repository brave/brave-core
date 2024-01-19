/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

namespace brave_ads::cbr {

namespace {

std::optional<challenge_bypass_ristretto::PublicKey> Create(
    const std::string& public_key_base64) {
  if (public_key_base64.empty()) {
    return std::nullopt;
  }

  return ValueOrLogError(
      challenge_bypass_ristretto::PublicKey::DecodeBase64(public_key_base64));
}

}  // namespace

PublicKey::PublicKey() = default;

PublicKey::PublicKey(const std::string& public_key_base64)
    : public_key_(Create(public_key_base64)) {}

PublicKey::PublicKey(const challenge_bypass_ristretto::PublicKey& public_key)
    : public_key_(public_key) {}

PublicKey::PublicKey(const PublicKey& other) = default;

PublicKey& PublicKey::operator=(const PublicKey& other) = default;

PublicKey::PublicKey(PublicKey&& other) noexcept = default;

PublicKey& PublicKey::operator=(PublicKey&& other) noexcept = default;

PublicKey::~PublicKey() = default;

bool PublicKey::operator==(const PublicKey& other) const {
  return EncodeBase64().value_or("") == other.EncodeBase64().value_or("");
}

bool PublicKey::operator!=(const PublicKey& other) const {
  return !(*this == other);
}

PublicKey PublicKey::DecodeBase64(const std::string& public_key_base64) {
  return PublicKey(public_key_base64);
}

std::optional<std::string> PublicKey::EncodeBase64() const {
  if (!public_key_ || !has_value()) {
    return std::nullopt;
  }

  return public_key_->EncodeBase64();
}

std::ostream& operator<<(std::ostream& os, const PublicKey& public_key) {
  os << public_key.EncodeBase64().value_or("");
  return os;
}

}  // namespace brave_ads::cbr
