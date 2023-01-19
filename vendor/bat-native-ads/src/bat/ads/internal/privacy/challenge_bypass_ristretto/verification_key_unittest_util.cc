/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key_unittest_util.h"

#include "base/check.h"

#include "absl/types/optional.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"

namespace ads::privacy::cbr {

VerificationKey GetVerificationKey() {
  const UnblindedToken unblinded_token = GetUnblindedToken();
  const absl::optional<VerificationKey> verification_key =
      unblinded_token.DeriveVerificationKey();
  DCHECK(verification_key);
  return *verification_key;
}

}  // namespace ads::privacy::cbr
