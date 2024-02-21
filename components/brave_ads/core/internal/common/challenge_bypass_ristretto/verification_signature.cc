/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"

namespace brave_ads::cbr {

namespace {

std::optional<challenge_bypass_ristretto::VerificationSignature> Create(
    const std::string& verification_signature_base64) {
  if (verification_signature_base64.empty()) {
    return std::nullopt;
  }

  return ValueOrLogError(
      challenge_bypass_ristretto::VerificationSignature::DecodeBase64(
          verification_signature_base64));
}

}  // namespace

VerificationSignature::VerificationSignature() = default;

VerificationSignature::VerificationSignature(
    const std::string& verification_signature_base64)
    : verification_signature_(Create(verification_signature_base64)) {}

VerificationSignature::VerificationSignature(
    const challenge_bypass_ristretto::VerificationSignature&
        verification_signature)
    : verification_signature_(verification_signature) {}

VerificationSignature::VerificationSignature(
    const VerificationSignature& other) = default;

VerificationSignature& VerificationSignature::operator=(
    const VerificationSignature& other) = default;

VerificationSignature::VerificationSignature(
    VerificationSignature&& other) noexcept = default;

VerificationSignature& VerificationSignature::operator=(
    VerificationSignature&& other) noexcept = default;

VerificationSignature::~VerificationSignature() = default;

bool VerificationSignature::operator==(
    const VerificationSignature& other) const {
  return EncodeBase64().value_or("") == other.EncodeBase64().value_or("");
}

bool VerificationSignature::operator!=(
    const VerificationSignature& other) const {
  return !(*this == other);
}

VerificationSignature VerificationSignature::DecodeBase64(
    const std::string& verification_signature_base64) {
  return VerificationSignature(verification_signature_base64);
}

std::optional<std::string> VerificationSignature::EncodeBase64() const {
  if (!verification_signature_ || !has_value()) {
    return std::nullopt;
  }

  return verification_signature_->EncodeBase64();
}

std::ostream& operator<<(std::ostream& os,
                         const VerificationSignature& verification_signature) {
  os << verification_signature.EncodeBase64().value_or("");
  return os;
}

}  // namespace brave_ads::cbr
