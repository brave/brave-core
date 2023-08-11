/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key_unittest_util.h"

#include "base/check.h"

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy::cbr {

VerificationKey GetVerificationKeyForTesting() {
  const UnblindedToken unblinded_token = GetUnblindedTokenForTesting();
  const absl::optional<VerificationKey> verification_key =
      unblinded_token.DeriveVerificationKey();
  CHECK(verification_key);
  return *verification_key;
}

}  // namespace brave_ads::privacy::cbr
