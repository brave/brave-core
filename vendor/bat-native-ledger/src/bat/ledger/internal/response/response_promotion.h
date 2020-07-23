/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RESPONSE_RESPONSE_PROMOTION_H_
#define BRAVELEDGER_RESPONSE_RESPONSE_PROMOTION_H_

#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"

namespace braveledger_response_util {

ledger::Result ParseClaimCreds(
    const ledger::UrlResponse& response,
    std::string* claim_id);

ledger::Result CheckFetchPromotions(const ledger::UrlResponse& response);

ledger::Result ParseFetchPromotions(
    const ledger::UrlResponse& response,
    ledger::PromotionList* list,
    std::vector<std::string>* corrupted_promotions);

ledger::Result CheckCorruptedPromotions(const ledger::UrlResponse& response);

ledger::Result CheckRedeemTokens(const ledger::UrlResponse& response);

}  // namespace braveledger_response_util

#endif  // BRAVELEDGER_RESPONSE_RESPONSE_PROMOTION_H_
