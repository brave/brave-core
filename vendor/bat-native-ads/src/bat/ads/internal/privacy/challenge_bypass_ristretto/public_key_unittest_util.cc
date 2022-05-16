/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key_unittest_util.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_unittest_constants.h"

namespace ads {
namespace privacy {
namespace cbr {

PublicKey GetPublicKey() {
  return PublicKey(kPublicKeyBase64);
}

PublicKey GetMismatchingPublicKey() {
  return PublicKey();
}

PublicKey GetInvalidPublicKey() {
  return PublicKey(kInvalidBase64);
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
