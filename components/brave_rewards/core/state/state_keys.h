/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_KEYS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_KEYS_H_

namespace brave_rewards::internal {
namespace state {

inline constexpr char kServerPublisherListStamp[] =
    "publisher_prefix_list_stamp";
inline constexpr char kUpholdAnonAddress[] =
    "uphold_anon_address";  // DEPRECATED
inline constexpr char kVersion[] = "version";
inline constexpr char kMinVisitTime[] = "ac.min_visit_time";
inline constexpr char kMinVisits[] = "ac.min_visits";
inline constexpr char kScoreA[] = "ac.score.a";
inline constexpr char kScoreB[] = "ac.score.b";
inline constexpr char kAutoContributeEnabled[] = "ac.enabled";
inline constexpr char kAutoContributeAmount[] = "ac.amount";
inline constexpr char kNextReconcileStamp[] = "ac.next_reconcile_stamp";
inline constexpr char kCreationStamp[] = "creation_stamp";
inline constexpr char kRecoverySeed[] = "wallet.seed";     // DEPRECATED
inline constexpr char kPaymentId[] = "wallet.payment_id";  // DEPRECATED
inline constexpr char kParameters[] = "parameters";
inline constexpr char kExternalWalletType[] = "external_wallet_type";
inline constexpr char kSelfCustodyAvailable[] = "self_custody_available";
inline constexpr char kSelfCustodyInviteDismissed[] =
    "self_custody_invite_dismissed";
inline constexpr char kWalletBrave[] = "wallets.brave";
inline constexpr char kWalletUphold[] = "wallets.uphold";
inline constexpr char kWalletBitflyer[] = "wallets.bitflyer";
inline constexpr char kWalletGemini[] = "wallets.gemini";
inline constexpr char kDeclaredGeo[] = "declared_geo";
inline constexpr char kWalletZebPay[] = "wallets.zebpay";
inline constexpr char kWalletSolana[] = "wallets.solana";

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_KEYS_H_
