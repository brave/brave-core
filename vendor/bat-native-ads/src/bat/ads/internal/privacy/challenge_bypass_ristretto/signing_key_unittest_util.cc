/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signing_key_unittest_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signing_key.h"

namespace ads::privacy::cbr {

SigningKey GetSigningKey() {
  return SigningKey(kSigningKeyBase64);
}

SigningKey GetInvalidSigningKey() {
  return SigningKey(kInvalidBase64);
}

}  // namespace ads::privacy::cbr
