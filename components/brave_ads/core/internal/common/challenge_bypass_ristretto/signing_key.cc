/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signing_key.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"

namespace brave_ads::cbr {

namespace {

std::optional<challenge_bypass_ristretto::SigningKey> Create(
    const std::string& signing_key_base64) {
  if (signing_key_base64.empty()) {
    return std::nullopt;
  }

  return ValueOrLogError(
      challenge_bypass_ristretto::SigningKey::DecodeBase64(signing_key_base64));
}

}  // namespace

SigningKey::SigningKey()
    : signing_key_(challenge_bypass_ristretto::SigningKey::Random()) {}

SigningKey::SigningKey(const std::string& signing_key_base64)
    : signing_key_(Create(signing_key_base64)) {}

SigningKey::SigningKey(
    const challenge_bypass_ristretto::SigningKey& signing_key)
    : signing_key_(signing_key) {}

SigningKey::~SigningKey() = default;

bool SigningKey::operator==(const SigningKey& other) const {
  return EncodeBase64().value_or("") == other.EncodeBase64().value_or("");
}

bool SigningKey::operator!=(const SigningKey& other) const {
  return !(*this == other);
}

SigningKey SigningKey::DecodeBase64(const std::string& signing_key_base64) {
  return SigningKey(signing_key_base64);
}

std::optional<std::string> SigningKey::EncodeBase64() const {
  if (!signing_key_ || !has_value()) {
    return std::nullopt;
  }

  return signing_key_->EncodeBase64();
}

std::optional<SignedToken> SigningKey::Sign(
    const BlindedToken& blinded_token) const {
  if (!signing_key_ || !has_value() || !blinded_token.has_value()) {
    return std::nullopt;
  }

  return ValueOrLogError<challenge_bypass_ristretto::SignedToken, SignedToken>(
      signing_key_->Sign(blinded_token.get()));
}

std::optional<UnblindedToken> SigningKey::RederiveUnblindedToken(
    const TokenPreimage& token_preimage) {
  if (!signing_key_ || !has_value() || !token_preimage.has_value()) {
    return std::nullopt;
  }

  return ValueOrLogError<challenge_bypass_ristretto::UnblindedToken,
                         UnblindedToken>(
      signing_key_->RederiveUnblindedToken(token_preimage.get()));
}

std::optional<PublicKey> SigningKey::GetPublicKey() {
  if (!signing_key_ || !has_value()) {
    return std::nullopt;
  }

  return PublicKey(signing_key_->GetPublicKey());
}

std::ostream& operator<<(std::ostream& os, const SigningKey& signing_key) {
  os << signing_key.EncodeBase64().value_or("");
  return os;
}

}  // namespace brave_ads::cbr
