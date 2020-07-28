/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_COMMON_PROMOTION_REQUESTS_H_
#define BRAVELEDGER_COMMON_PROMOTION_REQUESTS_H_

#include <string>
#include "bat/ledger/mojom_structs.h"

namespace braveledger_request_util {

std::string GetFetchPromotionUrl(
    const std::string& payment_id,
    const std::string& platform);

std::string ClaimCredsUrl(const std::string& promotion_id);

std::string FetchSignedCredsUrl(
    const std::string& promotion_id,
    const std::string& claim_id);

std::string GetRedeemTokensUrl();

std::string ReportClobberedClaimsUrl();

std::string GetTransferTokens();

std::string GetBatlossURL(const std::string& payment_id, const int32_t version);

std::string GetCreateWalletURL();

std::string GetRecoverWalletURL(const std::string& public_key);

std::string GetClaimWalletURL(const std::string& payment_id);

std::string GetBalanceWalletURL(const std::string& payment_id);

}  // namespace braveledger_request_util

#endif  // BRAVELEDGER_COMMON_PROMOTION_REQUESTS_H_
