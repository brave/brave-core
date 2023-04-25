/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_KEYS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_KEYS_H_

namespace brave_rewards::internal {
namespace log {

const char kACAddedToQueue[] = "ac_added_to_queue";
const char kDatabaseMigrated[] = "database_migrated";
const char kDeviceLimitReached[] = "device_limit_reached";
const char kFlaggedWallet[] = "flagged_wallet";
const char kKYCRequired[] = "kyc_required";
const char kMismatchedCountries[] = "mismatched_countries";
const char kMismatchedProviderAccounts[] = "mismatched_provider_accounts";
const char kPromotionVBATDrained[] = "promotion_vbat_drained";
const char kProviderUnavailable[] = "provider_unavailable";
const char kRecurringTipAdded[] = "recurring_tip_added";
const char kRecurringTipRemoved[] = "recurring_tip_removed";
const char kRegionNotSupported[] = "region_not_supported";
const char kRequestSignatureVerificationFailure[] =
    "request_signature_verification_failure";
const char kTransactionVerificationFailure[] =
    "transaction_verification_failure";
const char kWalletConnected[] = "wallet_connected";
const char kWalletCorrupted[] = "wallet_corrupted";
const char kWalletDisconnected[] = "wallet_disconnected";
const char kWalletStatusChange[] = "wallet_status_change";
const char kWalletVerified[] = "wallet_verified";

}  // namespace log
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_KEYS_H_
