/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RESPONSE_RESPONSE_SKU_H_
#define BRAVELEDGER_RESPONSE_RESPONSE_SKU_H_

#include <vector>

#include "bat/ledger/mojom_structs.h"

namespace braveledger_response_util {

ledger::Result CheckSendExternalTransaction(
    const ledger::UrlResponse& response);

ledger::SKUOrderPtr ParseOrderCreate(
    const ledger::UrlResponse& response,
    const std::vector<ledger::SKUOrderItem>& order_items);

ledger::Result CheckClaimSKUCreds(const ledger::UrlResponse& response);

ledger::Result CheckRedeemSKUTokens(
    const ledger::UrlResponse& response);

}  // namespace braveledger_response_util

#endif  // BRAVELEDGER_RESPONSE_RESPONSE_SKU_H_
