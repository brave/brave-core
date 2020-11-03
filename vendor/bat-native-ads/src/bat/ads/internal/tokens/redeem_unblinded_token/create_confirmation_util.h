/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_  // NOLINT

#include <string>

namespace ads {

struct ConfirmationInfo;

namespace privacy {
struct UnblindedTokenInfo;
}  // privacy

std::string CreateConfirmationRequestDTO(
    const ConfirmationInfo& confirmation);

std::string CreateCredential(
    const privacy::UnblindedTokenInfo& unblinded_token,
    const std::string& payload);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_  // NOLINT
