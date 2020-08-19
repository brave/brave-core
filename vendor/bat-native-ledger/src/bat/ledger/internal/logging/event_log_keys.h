/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LOGGING_EVENT_LOG_KEYS_H_
#define BRAVELEDGER_LOGGING_EVENT_LOG_KEYS_H_

#include <string>

namespace ledger {
namespace log {

const char kACAddedToQueue[] = "ac_added_to_queue";
const char kWalletConnected[] = "wallet_connected";
const char kWalletVerified[] = "wallet_verified";
const char kWalletDisconnected[] = "wallet_disconnected";
const char kRecurringTipAdded[] = "recurring_tip_added";
const char kRecurringTipRemoved[] = "recurring_tip_removed";
const char kDatabaseMigrated[] = "database_migrated";
const char kPromotionsClaimed[] = "promotion_claimed";

}  // namespace log
}  // namespace ledger

#endif  // BRAVELEDGER_LOGGING_EVENT_LOG_KEYS_H_
