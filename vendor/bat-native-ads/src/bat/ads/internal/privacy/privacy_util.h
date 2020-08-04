/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_PRIVACY_PRIVACY_UTIL_H_
#define BAT_ADS_INTERNAL_PRIVACY_PRIVACY_UTIL_H_

#include <vector>

#include "wrapper.hpp"

namespace ads {
namespace privacy {

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

std::vector<Token> GenerateTokens(const int count);

std::vector<BlindedToken> BlindTokens(
    const std::vector<Token>& tokens);

}  // namespace privacy
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_PRIVACY_PRIVACY_UTIL_H_
