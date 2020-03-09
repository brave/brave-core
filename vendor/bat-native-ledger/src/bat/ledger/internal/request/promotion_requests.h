/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_COMMON_PROMOTION_REQUESTS_H_
#define BRAVELEDGER_COMMON_PROMOTION_REQUESTS_H_

#include <string>

namespace braveledger_request_util {

std::string GetFetchPromotionUrl(
    const std::string& payment_id,
    const std::string& platform);

std::string ClaimTokensUrl(const std::string& promotion_id);

std::string FetchSignedTokensUrl(
    const std::string& promotion_id,
    const std::string& claim_id);

std::string GetReedemSuggestionsUrl();

std::string ReportClobberedClaimsUrl();

std::string GetTransferTokens();

}  // namespace braveledger_request_util

#endif  // BRAVELEDGER_COMMON_PROMOTION_REQUESTS_H_
