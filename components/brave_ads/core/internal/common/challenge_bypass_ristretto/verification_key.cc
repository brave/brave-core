/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"

#include "base/containers/span.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"

namespace brave_ads::cbr {

VerificationKey::VerificationKey(
    const challenge_bypass_ristretto::VerificationKey& verification_key)
    : verification_key_(verification_key) {}

std::optional<VerificationSignature> VerificationKey::Sign(
    const std::string& message) {
  return ValueOrLogError<challenge_bypass_ristretto::VerificationSignature,
                         VerificationSignature>(
      verification_key_.sign(base::as_bytes(base::make_span(message))));
}

bool VerificationKey::Verify(
    const VerificationSignature& verification_signature,
    const std::string& message) {
  if (!verification_signature.has_value()) {
    return false;
  }

  return verification_key_
      .verify(verification_signature.get(),
              base::as_bytes(base::make_span(message)))
      .value_or(false);
}

}  // namespace brave_ads::cbr
