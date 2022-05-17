/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signing_key.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace ads {
namespace privacy {
namespace cbr {

namespace {

absl::optional<challenge_bypass_ristretto::SigningKey> Create() {
  const challenge_bypass_ristretto::SigningKey raw_signing_key =
      challenge_bypass_ristretto::SigningKey::random();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_signing_key;
}

absl::optional<challenge_bypass_ristretto::SigningKey> Create(
    const std::string& signing_key_base64) {
  if (signing_key_base64.empty()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::SigningKey raw_signing_key =
      challenge_bypass_ristretto::SigningKey::decode_base64(signing_key_base64);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_signing_key;
}

}  // namespace

SigningKey::SigningKey() : signing_key_(Create()) {}

SigningKey::SigningKey(const std::string& signing_key_base64)
    : signing_key_(Create(signing_key_base64)) {}

SigningKey::SigningKey(
    const challenge_bypass_ristretto::SigningKey& signing_key)
    : signing_key_(signing_key) {}

SigningKey::SigningKey(const SigningKey& other) = default;

SigningKey::~SigningKey() = default;

bool SigningKey::operator==(const SigningKey& rhs) const {
  return EncodeBase64().value_or("") == rhs.EncodeBase64().value_or("");
}

bool SigningKey::operator!=(const SigningKey& rhs) const {
  return !(*this == rhs);
}

SigningKey SigningKey::DecodeBase64(const std::string& signing_key_base64) {
  return SigningKey(signing_key_base64);
}

absl::optional<std::string> SigningKey::EncodeBase64() const {
  if (!has_value()) {
    return absl::nullopt;
  }

  const std::string encoded_base64 = signing_key_->encode_base64();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return encoded_base64;
}

absl::optional<SignedToken> SigningKey::Sign(
    const BlindedToken& blinded_token) const {
  if (!has_value() || !blinded_token.has_value()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::SignedToken raw_signed_token =
      signing_key_->sign(blinded_token.get());
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return SignedToken(raw_signed_token);
}

absl::optional<UnblindedToken> SigningKey::RederiveUnblindedToken(
    const TokenPreimage& token_preimage) {
  if (!has_value() || !token_preimage.has_value()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::UnblindedToken raw_unblinded_token =
      signing_key_->rederive_unblinded_token(token_preimage.get());
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return UnblindedToken(raw_unblinded_token);
}

absl::optional<PublicKey> SigningKey::GetPublicKey() {
  if (!has_value()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::PublicKey raw_public_key =
      signing_key_->public_key();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return PublicKey(raw_public_key);
}

std::ostream& operator<<(std::ostream& os, const SigningKey& signing_key) {
  os << signing_key.EncodeBase64().value_or("");
  return os;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
