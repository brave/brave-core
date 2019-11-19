/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_COMMON_BIND_UTIL_H_
#define BRAVELEDGER_COMMON_BIND_UTIL_H_

#include <string>

#include "bat/ledger/mojom_structs.h"

/***
 * NOTICE!!!
 *
 * Add to this file ONLY conversion that you need when sending mojo object with
 * bind and this only applies for nested mojo structs. If you have single level
 * mojo struct std::bind works ok.
 */

namespace braveledger_bind_util {

std::string FromContributionQueueToString(ledger::ContributionQueuePtr info);

ledger::ContributionQueuePtr FromStringToContributionQueue(
    const std::string& data);

std::string FromPromotionToString(const ledger::PromotionPtr info);

ledger::PromotionPtr FromStringToPromotion(const std::string& data);

}  // namespace braveledger_bind_util

#endif  // BRAVELEDGER_COMMON_BIND_UTIL_H_
