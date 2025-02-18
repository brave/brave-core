/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_UTIL_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/challenge_bypass_ristretto/signed_token.h"

namespace brave_ads::cbr {

std::vector<challenge_bypass_ristretto::SignedToken> ToRawSignedTokens(
    const SignedTokenList& signed_tokens);

}  // namespace brave_ads::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_SIGNED_TOKEN_UTIL_H_
