/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature_unittest_util.h"

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"

namespace brave_ads::cbr::test {

VerificationSignature GetVerificationSignature() {
  return VerificationSignature(kVerificationSignatureBase64);
}

VerificationSignature GetInvalidVerificationSignature() {
  return VerificationSignature(kInvalidBase64);
}

}  // namespace brave_ads::cbr::test
