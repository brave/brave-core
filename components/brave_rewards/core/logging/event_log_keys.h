/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_KEYS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_KEYS_H_

namespace brave_rewards::internal {
namespace log {

inline constexpr char kACAddedToQueue[] = "ac_added_to_queue";
inline constexpr char kDatabaseMigrated[] = "database_migrated";
inline constexpr char kDeviceLimitReached[] = "device_limit_reached";
inline constexpr char kFlaggedWallet[] = "flagged_wallet";
inline constexpr char kKYCRequired[] = "kyc_required";
inline constexpr char kMismatchedCountries[] = "mismatched_countries";
inline constexpr char kMismatchedProviderAccounts[] =
    "mismatched_provider_accounts";
inline constexpr char kPromotionVBATDrained[] = "promotion_vbat_drained";
inline constexpr char kProviderUnavailable[] = "provider_unavailable";
inline constexpr char kRecurringTipAdded[] = "recurring_tip_added";
inline constexpr char kRecurringTipRemoved[] = "recurring_tip_removed";
inline constexpr char kRegionNotSupported[] = "region_not_supported";
inline constexpr char kRequestSignatureVerificationFailure[] =
    "request_signature_verification_failure";
inline constexpr char kTransactionVerificationFailure[] =
    "transaction_verification_failure";
inline constexpr char kWalletConnected[] = "wallet_connected";
inline constexpr char kWalletCorrupted[] = "wallet_corrupted";
inline constexpr char kWalletDisconnected[] = "wallet_disconnected";
inline constexpr char kWalletStatusChange[] = "wallet_status_change";
inline constexpr char kWalletVerified[] = "wallet_verified";

}  // namespace log
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LOGGING_EVENT_LOG_KEYS_H_
