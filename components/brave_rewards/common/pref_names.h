/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_

namespace brave_rewards::prefs {

// Used to enable/disable Rewards via a policy.
inline constexpr char kDisabledByPolicy[] = "brave.rewards.disabled_by_policy";

// Stores the "Rewards version" in which the user's Rewards account was created.
// This value is intended to be used to distinguish "legacy" users when Rewards
// features are migrated. Note that if the user has not yet created a Rewards
// account, this value will default to the empty string.
inline constexpr char kUserVersion[] = "brave.rewards.user_version";
inline constexpr char kCurrentUserVersion[] = "2.5";

inline constexpr char kHideButton[] =
    "brave.hide_brave_rewards_button";  // DEPRECATED
inline constexpr char kShowButton[] =
    "brave.show_brave_rewards_button";  // DEPRECATED
inline constexpr char kShowLocationBarButton[] =
    "brave.rewards.show_brave_rewards_button_in_location_bar";
inline constexpr char kEnabled[] = "brave.rewards.enabled";
inline constexpr char kDeclaredGeo[] = "brave.rewards.declared_geo";
inline constexpr char kNotifications[] = "brave.rewards.notifications";
inline constexpr char kNotificationTimerInterval[] =
    "brave.rewards.notification_timer_interval";
inline constexpr char kBackupNotificationInterval[] =
    "brave.rewards.backup_notification_interval";  // DEPRECATED
inline constexpr char kBackupSucceeded[] =
    "brave.rewards.backup_succeeded";  // DEPRECATED
inline constexpr char kUserHasFunded[] =
    "brave.rewards.user_has_funded";  // DEPRECATED
inline constexpr char kUserHasClaimedGrant[] =
    "brave.rewards.user_has_claimed_grant";  // DEPRECATED
inline constexpr char kAddFundsNotification[] =
    "brave.rewards.add_funds_notification";  // DEPRECATED
inline constexpr char kNotificationStartupDelay[] =
    "brave.rewards.notification_startup_delay";
inline constexpr char kExternalWallets[] =
    "brave.rewards.external_wallets";  // DEPRECATED

// Defined in core
inline constexpr char kServerPublisherListStamp[] =
    "brave.rewards.publisher_prefix_list_stamp";
inline constexpr char kUpholdAnonAddress[] =
    "brave.rewards.uphold_anon_address";  // DEPRECATED
inline constexpr char kBadgeText[] = "brave.rewards.badge_text";
inline constexpr char kUseRewardsStagingServer[] =
    "brave.rewards.use_staging_server";
inline constexpr char kExternalWalletType[] =
    "brave.rewards.external_wallet_type";
inline constexpr char kSelfCustodyAvailable[] =
    "brave.rewards.self_custody_available";
inline constexpr char kSelfCustodyInviteDismissed[] =
    "brave.rewards.self_custody_invite_dismissed";
inline constexpr char kP3APanelTriggerCount[] =
    "brave.rewards.p3a_panel_trigger_count";
inline constexpr char kPromotionLastFetchStamp[] =
    "brave.rewards.promotion_last_fetch_stamp";  // DEPRECATED
inline constexpr char kPromotionCorruptedMigrated[] =
    "brave.rewards.promotion_corrupted_migrated2";  // DEPRECATED
inline constexpr char kAnonTransferChecked[] =
    "brave.rewards.anon_transfer_checked";
inline constexpr char kVersion[] = "brave.rewards.version";
inline constexpr char kMinVisitTime[] = "brave.rewards.ac.min_visit_time";
inline constexpr char kMinVisits[] = "brave.rewards.ac.min_visits";
inline constexpr char kAllowNonVerified[] =
    "brave.rewards.ac.allow_non_verified";  // DEPRECATED
inline constexpr char kAllowVideoContribution[] =
    "brave.rewards.ac.allow_video_contributions";  // DEPRECATED
inline constexpr char kScoreA[] = "brave.rewards.ac.score.a";
inline constexpr char kScoreB[] = "brave.rewards.ac.score.b";
inline constexpr char kAutoContributeEnabled[] = "brave.rewards.ac.enabled";
inline constexpr char kAutoContributeAmount[] = "brave.rewards.ac.amount";
inline constexpr char kNextReconcileStamp[] =
    "brave.rewards.ac.next_reconcile_stamp";
inline constexpr char kCreationStamp[] = "brave.rewards.creation_stamp";
inline constexpr char kRecoverySeed[] =
    "brave.rewards.wallet.seed";  // DEPRECATED
inline constexpr char kPaymentId[] =
    "brave.rewards.wallet.payment_id";  // DEPRECATED
inline constexpr char kInlineTipButtonsEnabled[] =
    "brave.rewards.inline_tip_buttons_enabled";  // DEPRECATED
inline constexpr char kInlineTipRedditEnabled[] =
    "brave.rewards.inline_tip.reddit";  // DEPRECATED
inline constexpr char kInlineTipTwitterEnabled[] =
    "brave.rewards.inline_tip.twitter";  // DEPRECATED
inline constexpr char kInlineTipGithubEnabled[] =
    "brave.rewards.inline_tip.github";  // DEPRECATED
inline constexpr char kParameters[] = "brave.rewards.parameters";
inline constexpr char kFetchOldBalance[] =
    "brave.rewards.fetch_old_balance";  // DEPRECATED
inline constexpr char kEmptyBalanceChecked[] =
    "brave.rewards.empty_balance_checked";  // DEPRECATED
inline constexpr char kWalletBrave[] = "brave.rewards.wallets.brave";
inline constexpr char kWalletUphold[] = "brave.rewards.wallets.uphold";
inline constexpr char kWalletBitflyer[] = "brave.rewards.wallets.bitflyer";
inline constexpr char kWalletGemini[] = "brave.rewards.wallets.gemini";
inline constexpr char kWalletZebPay[] = "brave.rewards.wallets.zebpay";
inline constexpr char kWalletSolana[] = "brave.rewards.wallets.solana";
inline constexpr char kWalletCreationEnvironment[] =
    "brave.rewards.wallet_creation_environment";
inline constexpr char kTosVersion[] = "brave.rewards.tos_version";

inline constexpr char kRewardsPageViewCount[] =
    "brave.rewards.p3a_page_view_count";
inline constexpr char kRewardsDesktopPanelViewCount[] =
    "brave.rewards.p3a_desktop_panel_view_count";

// deprecated p3a prefs
inline constexpr char kAdsWereDisabled[] = "brave.brave_ads.were_disabled";
inline constexpr char kHasAdsP3AState[] = "brave.brave_ads.has_p3a_state";
inline constexpr char kAdsEnabledTimeDelta[] =
    "brave.rewards.ads_enabled_time_delta";
inline constexpr char kAdsEnabledTimestamp[] =
    "brave.rewards.ads_enabled_timestamp";

}  // namespace brave_rewards::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_PREF_NAMES_H_
