/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_rewards {
namespace prefs {

const char kHideBraveRewardsButton[] = "brave.hide_brave_rewards_button";
const char kBraveRewardsEnabled[] = "brave.rewards.enabled";
const char kRewardsNotifications[] = "brave.rewards.notifications";
const char kRewardsNotificationTimerInterval[]=
    "brave.rewards.notification_timer_interval";
const char kRewardsBackupNotificationInterval[] =
    "brave.rewards.backup_notification_interval";
const char kRewardsBackupSucceeded[] = "brave.rewards.backup_succeeded";
const char kRewardsUserHasFunded[] = "brave.rewards.user_has_funded";
const char kRewardsAddFundsNotification[] =
    "brave.rewards.add_funds_notification";
const char kRewardsNotificationStartupDelay[] =
    "brave.rewards.notification_startup_delay";
const char kRewardsExternalWallets[] = "brave.rewards.external_wallets";
const char kStateServerPublisherListStamp[] =
    "brave.rewards.publisher_prefix_list_stamp";
const char kStateUpholdAnonAddress[] =
    "brave.rewards.uphold_anon_address";
const char kRewardsBadgeText[] = "brave.rewards.badge_text";
const char kUseRewardsStagingServer[] = "brave.rewards.use_staging_server";
const char kStatePromotionLastFetchStamp[] =
    "brave.rewards.promotion_last_fetch_stamp";
const char kStatePromotionCorruptedMigrated[] =
    "brave.rewards.promotion_corrupted_migrated2";
const char kStateAnonTransferChecked[] =  "brave.rewards.anon_transfer_checked";
const char kStateVersion[] =  "brave.rewards.version";
const char kStateMinVisitTime[] =  "brave.rewards.ac.min_visit_time";
const char kStateMinVisits[] =  "brave.rewards.ac.min_visits";
const char kStateAllowNonVerified[] =  "brave.rewards.ac.allow_non_verified";
const char kStateAllowVideoContribution[] =
    "brave.rewards.ac.allow_video_contributions";
const char kStateScoreA[] = "brave.rewards.ac.score.a";
const char kStateScoreB[] = "brave.rewards.ac.score.b";
const char kStateAutoContributeEnabled[] = "brave.rewards.ac.enabled";
const char kStateAutoContributeAmount[] = "brave.rewards.ac.amount";
const char kStateNextReconcileStamp[] = "brave.rewards.ac.next_reconcile_stamp";
const char kStateCreationStamp[] = "brave.rewards.creation_stamp";
const char kStateRecoverySeed[] = "brave.rewards.wallet.seed";
const char kStatePaymentId[] = "brave.rewards.wallet.payment_id";
const char kStateInlineTipRedditEnabled[] = "brave.rewards.inline_tip.reddit";
const char kStateInlineTipTwitterEnabled[] = "brave.rewards.inline_tip.twitter";
const char kStateInlineTipGithubEnabled[] = "brave.rewards.inline_tip.github";
const char kStateParametersRate[] = "brave.rewards.parameters.rate";
const char kStateParametersAutoContributeChoice[] =
    "brave.rewards.parameters.ac.choice";
const char kStateParametersAutoContributeChoices[] =
    "brave.rewards.parameters.ac.choices";
const char kStateParametersTipChoices[] =
    "brave.rewards.parameters.tip.choices";
const char kStateParametersMonthlyTipChoices[] =
    "brave.rewards.parameters.tip.monthly_choices";
const char kStateFetchOldBalance[] =
    "brave.rewards.fetch_old_balance";
const char kStateEmptyBalanceChecked[] =
    "brave.rewards.empty_balance_checked";
}  // namespace prefs
}  // namespace brave_rewards
