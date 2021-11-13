/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_

#include <string>

namespace ads {

namespace privacy {
struct UnblindedTokenInfo;
}  // namespace privacy

struct ConfirmationInfo;

std::string CreateConfirmationRequestDTO(const ConfirmationInfo& confirmation);

std::string CreateCredential(const privacy::UnblindedTokenInfo& unblinded_token,
                             const std::string& payload);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_UTIL_H_
