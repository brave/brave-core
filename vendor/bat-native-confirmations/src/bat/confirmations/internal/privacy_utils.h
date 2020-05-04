/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_PRIVACY_UTILS_H_
#define BAT_CONFIRMATIONS_INTERNAL_PRIVACY_UTILS_H_

#include <vector>

#include "wrapper.hpp"  // NOLINT

namespace confirmations {
namespace privacy {

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

std::vector<Token> GenerateTokens(const int count);

std::vector<BlindedToken> BlindTokens(
    const std::vector<Token>& tokens);

}  // namespace privacy
}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_PRIVACY_UTILS_H_
