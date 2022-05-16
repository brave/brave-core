/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"

namespace ads {
namespace privacy {
namespace cbr {

VerificationKey::VerificationKey(
    const challenge_bypass_ristretto::VerificationKey& verification_key)
    : verification_key_(verification_key) {}

VerificationKey::~VerificationKey() = default;

absl::optional<VerificationSignature> VerificationKey::Sign(
    const std::string& message) {
  const challenge_bypass_ristretto::VerificationSignature
      raw_verification_signature = verification_key_.sign(message);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return VerificationSignature(raw_verification_signature);
}

bool VerificationKey::Verify(
    const VerificationSignature& verification_signature,
    const std::string& message) {
  if (!verification_signature.has_value()) {
    return false;
  }

  return verification_key_.verify(verification_signature.get(), message);
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
