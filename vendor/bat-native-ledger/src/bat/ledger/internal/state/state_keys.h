/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_KEYS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_KEYS_H_

#include <string>

namespace ledger {
namespace state {

const char kServerPublisherListStamp[] = "publisher_prefix_list_stamp";
const char kUpholdAnonAddress[] = "uphold_anon_address";  // DEPRECATED
const char kPromotionLastFetchStamp[] = "promotion_last_fetch_stamp";
const char kPromotionCorruptedMigrated[] =
    "promotion_corrupted_migrated2";
const char kAnonTransferChecked[] = "anon_transfer_checked";
const char kVersion[] = "version";
const char kMinVisitTime[] = "ac.min_visit_time";
const char kMinVisits[] = "ac.min_visits";
const char kAllowNonVerified[] = "ac.allow_non_verified";
const char kAllowVideoContribution[] = "ac.allow_video_contributions";
const char kScoreA[] = "ac.score.a";
const char kScoreB[] = "ac.score.b";
const char kAutoContributeEnabled[] = "ac.enabled";
const char kAutoContributeAmount[] = "ac.amount";
const char kNextReconcileStamp[] = "ac.next_reconcile_stamp";
const char kCreationStamp[] = "creation_stamp";
const char kRecoverySeed[] = "wallet.seed";  // DEPRECATED
const char kPaymentId[] = "wallet.payment_id";  // DEPRECATED
const char kInlineTipRedditEnabled[] = "inline_tip.reddit";
const char kInlineTipTwitterEnabled[] = "inline_tip.twitter";
const char kInlineTipGithubEnabled[] = "inline_tip.github";
const char kParametersRate[] = "parameters.rate";
const char kParametersAutoContributeChoice[] = "parameters.ac.choice";
const char kParametersAutoContributeChoices[] = "parameters.ac.choices";
const char kParametersTipChoices[] = "parameters.tip.choices";
const char kParametersMonthlyTipChoices[] =
    "parameters.tip.monthly_choices";
const char kFetchOldBalance[] = "fetch_old_balance";
const char kEmptyBalanceChecked[] = "empty_balance_checked";
const char kBAPReported[] = "bap_reported";
const char kWalletBrave[]  ="wallets.brave";
const char kWalletUphold[] = "wallets.uphold";
const char kWalletBitflyer[] = "wallets.bitflyer";

}  // namespace state
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_STATE_STATE_KEYS_H_
