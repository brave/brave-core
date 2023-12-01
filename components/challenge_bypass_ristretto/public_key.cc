/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/public_key.h"

#include <utility>

namespace challenge_bypass_ristretto {

PublicKey::PublicKey(CxxPublicKeyBox raw)
    : raw_(base::MakeRefCounted<CxxPublicKeyData>(
          CxxPublicKeyValueOrResult(std::move(raw)))) {}

PublicKey::PublicKey(CxxPublicKeyResultBox raw)
    : raw_(base::MakeRefCounted<CxxPublicKeyData>(
          CxxPublicKeyValueOrResult(std::move(raw)))) {}

PublicKey::PublicKey(const PublicKey& other) = default;

PublicKey& PublicKey::operator=(const PublicKey& other) = default;

PublicKey::PublicKey(PublicKey&& other) noexcept = default;

PublicKey& PublicKey::operator=(PublicKey&& other) noexcept = default;

PublicKey::~PublicKey() = default;

base::expected<PublicKey, std::string> PublicKey::DecodeBase64(
    const std::string& encoded) {
  CxxPublicKeyResultBox raw_public_key_result(
      cbr_cxx::decode_base64_public_key(encoded));

  if (!raw_public_key_result->is_ok()) {
    return base::unexpected("Failed to decode public key");
  }

  return PublicKey(std::move(raw_public_key_result));
}

std::string PublicKey::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool PublicKey::operator==(const PublicKey& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
