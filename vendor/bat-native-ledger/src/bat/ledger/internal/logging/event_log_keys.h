/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LOGGING_EVENT_LOG_KEYS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LOGGING_EVENT_LOG_KEYS_H_

#include <string>

namespace ledger {
namespace log {

const char kACAddedToQueue[] = "ac_added_to_queue";
const char kDatabaseMigrated[] = "database_migrated";
const char kDeviceLimitReached[] = "device_limit_reached";
const char kMismatchedProviderAccounts[] = "mismatched_provider_accounts";
const char kPromotionsClaimed[] = "promotion_claimed";
const char kRecurringTipAdded[] = "recurring_tip_added";
const char kRecurringTipRemoved[] = "recurring_tip_removed";
const char kWalletConnected[] = "wallet_connected";
const char kWalletDisconnected[] = "wallet_disconnected";
const char kWalletRecovered[] = "wallet_recovered";
const char kWalletStatusChange[] = "wallet_status_change";
const char kWalletVerified[] = "wallet_verified";

}  // namespace log
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_LOGGING_EVENT_LOG_KEYS_H_
