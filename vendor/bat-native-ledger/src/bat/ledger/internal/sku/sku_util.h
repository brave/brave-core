/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_SKU_SKU_UTIL_H_
#define BRAVELEDGER_SKU_SKU_UTIL_H_

#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"

namespace braveledger_sku {

ledger::SKUOrderPtr ParseOrderCreateResponse(
    const std::string& response,
    const std::vector<ledger::SKUOrderItem>& order_items);

std::string ConvertTransactionTypeToString(
    const ledger::SKUTransactionType type);

}  // namespace braveledger_sku

#endif  // BRAVELEDGER_SKU_SKU_UTIL_H_
