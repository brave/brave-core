/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_rewards {
namespace prefs {

const char kHideButton[] = "brave.hide_brave_rewards_button";
const char kEnabled[] = "brave.rewards.enabled";
const char kNotifications[] = "brave.rewards.notifications";
const char kNotificationTimerInterval[]=
    "brave.rewards.notification_timer_interval";
const char kBackupNotificationInterval[] =
    "brave.rewards.backup_notification_interval";
const char kBackupSucceeded[] = "brave.rewards.backup_succeeded";
const char kUserHasFunded[] = "brave.rewards.user_has_funded";
const char kAddFundsNotification[] =
    "brave.rewards.add_funds_notification";
const char kNotificationStartupDelay[] =
    "brave.rewards.notification_startup_delay";
const char kExternalWallets[] = "brave.rewards.external_wallets";
const char kServerPublisherListStamp[] =
    "brave.rewards.publisher_prefix_list_stamp";
const char kUpholdAnonAddress[] =
    "brave.rewards.uphold_anon_address";
const char kBadgeText[] = "brave.rewards.badge_text";
const char kUseRewardsStagingServer[] = "brave.rewards.use_staging_server";
const char kOnboarded[] = "brave.rewards.onboarded";
const char kPromotionLastFetchStamp[] =
    "brave.rewards.promotion_last_fetch_stamp";
const char kPromotionCorruptedMigrated[] =
    "brave.rewards.promotion_corrupted_migrated2";
const char kAnonTransferChecked[] =  "brave.rewards.anon_transfer_checked";
const char kVersion[] =  "brave.rewards.version";
const char kMinVisitTime[] =  "brave.rewards.ac.min_visit_time";
const char kMinVisits[] =  "brave.rewards.ac.min_visits";
const char kAllowNonVerified[] =  "brave.rewards.ac.allow_non_verified";
const char kAllowVideoContribution[] =
    "brave.rewards.ac.allow_video_contributions";
const char kScoreA[] = "brave.rewards.ac.score.a";
const char kScoreB[] = "brave.rewards.ac.score.b";
const char kAutoContributeEnabled[] = "brave.rewards.ac.enabled";
const char kAutoContributeAmount[] = "brave.rewards.ac.amount";
const char kNextReconcileStamp[] = "brave.rewards.ac.next_reconcile_stamp";
const char kCreationStamp[] = "brave.rewards.creation_stamp";
const char kRecoverySeed[] = "brave.rewards.wallet.seed";
const char kPaymentId[] = "brave.rewards.wallet.payment_id";
const char kInlineTipRedditEnabled[] = "brave.rewards.inline_tip.reddit";
const char kInlineTipTwitterEnabled[] = "brave.rewards.inline_tip.twitter";
const char kInlineTipGithubEnabled[] = "brave.rewards.inline_tip.github";
const char kParametersRate[] = "brave.rewards.parameters.rate";
const char kParametersAutoContributeChoice[] =
    "brave.rewards.parameters.ac.choice";
const char kParametersAutoContributeChoices[] =
    "brave.rewards.parameters.ac.choices";
const char kParametersTipChoices[] =
    "brave.rewards.parameters.tip.choices";
const char kParametersMonthlyTipChoices[] =
    "brave.rewards.parameters.tip.monthly_choices";
const char kFetchOldBalance[] =
    "brave.rewards.fetch_old_balance";
const char kEmptyBalanceChecked[] =
    "brave.rewards.empty_balance_checked";
const char kBAPReported[] = "brave.rewards.bap_reported";
const char kWalletBrave[] =
    "brave.rewards.wallets.brave";
const char kWalletUphold[] =
    "brave.rewards.wallets.uphold";
const char kWalletBitflyer[] = "brave.rewards.wallets.bitflyer";

}  // namespace prefs
}  // namespace brave_rewards
