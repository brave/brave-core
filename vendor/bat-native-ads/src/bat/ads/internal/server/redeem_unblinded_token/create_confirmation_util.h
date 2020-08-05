/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_  // NOLINT
#define BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_  // NOLINT

#include <string>

#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

namespace ads {

std::string CreateConfirmationRequestDTO(
    const ConfirmationInfo& confirmation);

std::string CreateCredential(
    const privacy::UnblindedTokenInfo& unblinded_token,
    const std::string& payload);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_  // NOLINT
