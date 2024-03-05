/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_KEYS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_KEYS_H_

namespace brave_rewards::internal {
namespace state {

const char kServerPublisherListStamp[] = "publisher_prefix_list_stamp";
const char kUpholdAnonAddress[] = "uphold_anon_address";  // DEPRECATED
const char kVersion[] = "version";
const char kMinVisitTime[] = "ac.min_visit_time";
const char kMinVisits[] = "ac.min_visits";
const char kScoreA[] = "ac.score.a";
const char kScoreB[] = "ac.score.b";
const char kAutoContributeEnabled[] = "ac.enabled";
const char kAutoContributeAmount[] = "ac.amount";
const char kNextReconcileStamp[] = "ac.next_reconcile_stamp";
const char kCreationStamp[] = "creation_stamp";
const char kRecoverySeed[] = "wallet.seed";     // DEPRECATED
const char kPaymentId[] = "wallet.payment_id";  // DEPRECATED
const char kParametersRate[] = "parameters.rate";
const char kParametersAutoContributeChoice[] = "parameters.ac.choice";
const char kParametersAutoContributeChoices[] = "parameters.ac.choices";
const char kParametersTipChoices[] = "parameters.tip.choices";
const char kParametersMonthlyTipChoices[] = "parameters.tip.monthly_choices";
const char kParametersPayoutStatus[] = "parameters.payout_status";
const char kParametersWalletProviderRegions[] =
    "parameters.wallet_provider_regions";
const char kParametersVBatDeadline[] = "parameters.vbat_deadline";
const char kParametersVBatExpired[] = "parameters.vbat_expired";
const char kParametersTosVersion[] = "parameters.tos_version";
const char kExternalWalletType[] = "external_wallet_type";
const char kSelfCustodyAvailable[] = "self_custody_available";
const char kSelfCustodyInviteDismissed[] = "self_custody_invite_dismissed";
const char kWalletBrave[] = "wallets.brave";
const char kWalletUphold[] = "wallets.uphold";
const char kWalletBitflyer[] = "wallets.bitflyer";
const char kWalletGemini[] = "wallets.gemini";
const char kDeclaredGeo[] = "declared_geo";
const char kWalletZebPay[] = "wallets.zebpay";
const char kWalletSolana[] = "wallets.solana";

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_KEYS_H_
