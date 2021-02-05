/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_PRIVACY_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_PRIVACY_UTIL_H_

#include <vector>

#include "wrapper.hpp"

namespace ads {
namespace privacy {

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::Token;

std::vector<BlindedToken> BlindTokens(const std::vector<Token>& tokens);

}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_PRIVACY_UTIL_H_
