/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_UNITTEST_UTIL_H_

#include <vector>

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"

namespace ads {
namespace privacy {
namespace cbr {

BlindedToken GetBlindedToken();
std::vector<BlindedToken> GetBlindedTokens();

BlindedToken GetInvalidBlindedToken();
std::vector<BlindedToken> GetInvalidBlindedTokens();

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BLINDED_TOKEN_UNITTEST_UTIL_H_
