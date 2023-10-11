/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_
#define BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace p3a {

// Allowlist for histograms that we collect.
// A metric must be listed here to be reported.
//
// Please keep them properly sorted within their categories.
//
// Could be replaced with something built dynamically,
// although so far we've found it a useful review point.
//
// TODO(iefremov) Clean up obsolete metrics.
//
// clang-format off
constexpr inline auto kCollectedTypicalHistograms =
  base::MakeFixedFlatSet<std::string_view>({
    "Brave.AIChat.AvgPromptCount",
    "Brave.AIChat.ChatCount",
    "Brave.AIChat.Enabled",
    "Brave.Core.BookmarksCountOnProfileLoad.2",
    "Brave.Core.CrashReportsEnabled",
    "Brave.Core.DomainsLoaded",
    "Brave.Core.IsDefault",
    "Brave.Core.LastTimeIncognitoUsed",
    "Brave.Core.NumberOfExtensions",
    "Brave.Core.PagesLoaded",
    "Brave.Core.TabCount",
    "Brave.Core.TorEverUsed",
    "Brave.Core.WeeklyUsage",
    "Brave.Core.WindowCount.2",
    "Brave.DNS.AutoSecureRequests",
    "Brave.DNS.SecureSetting",
    "Brave.Importer.ImporterSource.2",
    "Brave.NTP.CustomizeUsageStatus",
    "Brave.NTP.NewTabsCreated.2",
    "Brave.NTP.SponsoredImagesEnabled",
    "Brave.NTP.SponsoredNewTabsCreated",
    "Brave.Omnibox.SearchCount.2",
    "Brave.P3A.SentAnswersCount",
    "Brave.Playlist.FirstTimeOffset",
    "Brave.Playlist.NewUserReturning",
    "Brave.Playlist.UsageDaysInWeek",
    "Brave.Rewards.AdTypesEnabled",
    "Brave.Rewards.AutoContributionsState.3",
    "Brave.Rewards.EnabledSource",
    "Brave.Rewards.InlineTipTrigger",
    "Brave.Rewards.TipsState.2",
    "Brave.Rewards.ToolbarButtonTrigger",
    "Brave.Rewards.WalletBalance.3",
    "Brave.Rewards.WalletState",
    "Brave.Savings.BandwidthSavingsMB",
    "Brave.Search.DefaultEngine.4",
    "Brave.Search.Promo.BannerA",
    "Brave.Search.Promo.BannerB",
    "Brave.Search.Promo.BannerC",
    "Brave.Search.Promo.BannerD",
    "Brave.Search.Promo.Button",
    "Brave.Search.Promo.NewTabPage",
    "Brave.Search.QueriesBeforeChurn",
    "Brave.Search.SwitchEngine",
    "Brave.Search.WebDiscoveryEnabled",
    "Brave.Shields.AdBlockSetting",
    "Brave.Shields.CookieListEnabled",
    "Brave.Shields.CookieListPrompt",
    "Brave.Shields.DomainAdsSettingsAboveGlobal",
    "Brave.Shields.DomainAdsSettingsBelowGlobal",
    "Brave.Shields.DomainFingerprintSettingsAboveGlobal",
    "Brave.Shields.DomainFingerprintSettingsBelowGlobal",
    "Brave.Shields.FingerprintBlockSetting",
    "Brave.Shields.UsageStatus",
    "Brave.Sidebar.Enabled",
    "Brave.SpeedReader.Enabled",
    "Brave.SpeedReader.ToggleCount",
    "Brave.Today.DirectFeedsTotal",
    "Brave.Today.LastUsageTime",
    "Brave.Today.NewUserReturning",
    "Brave.Today.WeeklyAddedDirectFeedsCount",
    "Brave.Today.WeeklyDisplayAdsViewedCount",
    "Brave.Today.WeeklySessionCount",
    "Brave.Today.WeeklyTotalCardViews",
    "Brave.Toolbar.FrequentMenuGroup",
    "Brave.Toolbar.MenuDismissRate",
    "Brave.Toolbar.MenuOpens",
    "Brave.Sync.Status.2",
    "Brave.Sync.ProgressTokenEverReset",
    "Brave.VerticalTabs.GroupTabs",
    "Brave.VerticalTabs.OpenTabs",
    "Brave.VerticalTabs.PinnedTabs",
    "Brave.VPN.NewUserReturning",
    "Brave.VPN.DaysInMonthUsed",
    "Brave.VPN.LastUsageTime",
    "Brave.Wallet.ActiveEthAccounts",
    "Brave.Wallet.ActiveFilAccounts",
    "Brave.Wallet.ActiveSolAccounts",
    "Brave.Wallet.EthProvider.4",
    "Brave.Wallet.EthTransactionSent",
    "Brave.Wallet.FilTransactionSent",
    "Brave.Wallet.KeyringCreated",
    "Brave.Wallet.LastUsageTime",
    "Brave.Wallet.NewUserBalance",
    "Brave.Wallet.NewUserReturning",
    "Brave.Wallet.NFTCount",
    "Brave.Wallet.NFTDiscoveryEnabled",
    "Brave.Wallet.NFTNewUser",
    "Brave.Wallet.OnboardingConversion.3",
    "Brave.Wallet.SolProvider.2",
    "Brave.Wallet.SolTransactionSent",
    "Brave.Wallet.UsageWeekly",
    "Brave.Welcome.InteractionStatus.2",

    // IPFS
    "Brave.IPFS.IPFSCompanionInstalled",
    "Brave.IPFS.DetectionPromptCount",
    "Brave.IPFS.GatewaySetting",
    "Brave.IPFS.DaemonRunTime",
    "Brave.IPFS.LocalNodeRetention",

    // P2A
    // Ad Opportunities
    "Brave.P2A.ad_notification.opportunities",
    "Brave.P2A.ad_notification.opportunities_per_segment.architecture",
    "Brave.P2A.ad_notification.opportunities_per_segment.artsentertainment",
    "Brave.P2A.ad_notification.opportunities_per_segment.automotive",
    "Brave.P2A.ad_notification.opportunities_per_segment.business",
    "Brave.P2A.ad_notification.opportunities_per_segment.careers",
    "Brave.P2A.ad_notification.opportunities_per_segment.cellphones",
    "Brave.P2A.ad_notification.opportunities_per_segment.crypto",
    "Brave.P2A.ad_notification.opportunities_per_segment.education",
    "Brave.P2A.ad_notification.opportunities_per_segment.familyparenting",
    "Brave.P2A.ad_notification.opportunities_per_segment.fashion",
    "Brave.P2A.ad_notification.opportunities_per_segment.folklore",
    "Brave.P2A.ad_notification.opportunities_per_segment.fooddrink",
    "Brave.P2A.ad_notification.opportunities_per_segment.gaming",
    "Brave.P2A.ad_notification.opportunities_per_segment.healthfitness",
    "Brave.P2A.ad_notification.opportunities_per_segment.history",
    "Brave.P2A.ad_notification.opportunities_per_segment.hobbiesinterests",
    "Brave.P2A.ad_notification.opportunities_per_segment.home",
    "Brave.P2A.ad_notification.opportunities_per_segment.law",
    "Brave.P2A.ad_notification.opportunities_per_segment.military",
    "Brave.P2A.ad_notification.opportunities_per_segment.other",
    "Brave.P2A.ad_notification.opportunities_per_segment.personalfinance",
    "Brave.P2A.ad_notification.opportunities_per_segment.pets",
    "Brave.P2A.ad_notification.opportunities_per_segment.realestate",
    "Brave.P2A.ad_notification.opportunities_per_segment.science",
    "Brave.P2A.ad_notification.opportunities_per_segment.sports",
    "Brave.P2A.ad_notification.opportunities_per_segment.technologycomputing",
    "Brave.P2A.ad_notification.opportunities_per_segment.travel",
    "Brave.P2A.ad_notification.opportunities_per_segment.weather",
    "Brave.P2A.ad_notification.opportunities_per_segment.untargeted",
    "Brave.P2A.inline_content_ad.opportunities",
    "Brave.P2A.new_tab_page_ad.opportunities",
    // Ad Impressions
    "Brave.P2A.ad_notification.impressions",
    "Brave.P2A.ad_notification.impressions_per_segment.architecture",
    "Brave.P2A.ad_notification.impressions_per_segment.artsentertainment",
    "Brave.P2A.ad_notification.impressions_per_segment.automotive",
    "Brave.P2A.ad_notification.impressions_per_segment.business",
    "Brave.P2A.ad_notification.impressions_per_segment.careers",
    "Brave.P2A.ad_notification.impressions_per_segment.cellphones",
    "Brave.P2A.ad_notification.impressions_per_segment.crypto",
    "Brave.P2A.ad_notification.impressions_per_segment.education",
    "Brave.P2A.ad_notification.impressions_per_segment.familyparenting",
    "Brave.P2A.ad_notification.impressions_per_segment.fashion",
    "Brave.P2A.ad_notification.impressions_per_segment.folklore",
    "Brave.P2A.ad_notification.impressions_per_segment.fooddrink",
    "Brave.P2A.ad_notification.impressions_per_segment.gaming",
    "Brave.P2A.ad_notification.impressions_per_segment.healthfitness",
    "Brave.P2A.ad_notification.impressions_per_segment.history",
    "Brave.P2A.ad_notification.impressions_per_segment.hobbiesinterests",
    "Brave.P2A.ad_notification.impressions_per_segment.home",
    "Brave.P2A.ad_notification.impressions_per_segment.law",
    "Brave.P2A.ad_notification.impressions_per_segment.military",
    "Brave.P2A.ad_notification.impressions_per_segment.other",
    "Brave.P2A.ad_notification.impressions_per_segment.personalfinance",
    "Brave.P2A.ad_notification.impressions_per_segment.pets",
    "Brave.P2A.ad_notification.impressions_per_segment.realestate",
    "Brave.P2A.ad_notification.impressions_per_segment.science",
    "Brave.P2A.ad_notification.impressions_per_segment.sports",
    "Brave.P2A.ad_notification.impressions_per_segment.technologycomputing",
    "Brave.P2A.ad_notification.impressions_per_segment.travel",
    "Brave.P2A.ad_notification.impressions_per_segment.weather",
    "Brave.P2A.ad_notification.impressions_per_segment.untargeted",
    "Brave.P2A.inline_content_ad.impressions",
    "Brave.P2A.new_tab_page_ad.impressions"
});

constexpr inline auto kCollectedSlowHistograms =
  base::MakeFixedFlatSet<std::string_view>({
    "Brave.Accessibility.DisplayZoomEnabled",
    "Brave.Core.DocumentsDirectorySizeMB",
    "Brave.Core.ProfileCount",
    "Brave.Core.UsageMonthly",
    "Brave.General.BottomBarLocation",
    "Brave.P3A.TestSlowMetric",
    "Brave.Playlist.LastUsageTime",
    "Brave.PrivacyHub.IsEnabled",
    "Brave.PrivacyHub.Views",
    "Brave.ReaderMode.NumberReaderModeActivated",
    "Brave.Rewards.TipsSent.2",
    "Brave.Sync.EnabledTypes",
    "Brave.Sync.SyncedObjectsCount",
    "Brave.Today.UsageMonthly",
    "Brave.Toolbar.ForwardNavigationAction",
    "Brave.Wallet.UsageMonthly"
});

constexpr inline auto kCollectedExpressHistograms =
  base::MakeFixedFlatSet<std::string_view>({
    "Brave.AIChat.UsageDaily",
    "Brave.Core.UsageDaily",
    "Brave.Rewards.EnabledInstallationTime",
    "Brave.Today.IsEnabled",
    "Brave.Today.UsageDaily",
    "Brave.Uptime.BrowserOpenTime",
    "Brave.Wallet.UsageDaily"
});

// List of metrics that should only be sent once per latest histogram update.
// Once the metric value has been sent, the value will be removed from the log store.
constexpr inline auto kEphemeralHistograms =
  base::MakeFixedFlatSet<std::string_view>({
    "Brave.AIChat.AvgPromptCount",
    "Brave.AIChat.ChatCount",
    "Brave.AIChat.UsageDaily",
    "Brave.Playlist.UsageDaysInWeek",
    "Brave.Playlist.FirstTimeOffset",
    "Brave.PrivacyHub.Views",
    "Brave.Rewards.EnabledInstallationTime",
    "Brave.Rewards.EnabledSource",
    "Brave.Rewards.InlineTipTrigger",
    "Brave.Rewards.TipsSent",
    "Brave.Rewards.ToolbarButtonTrigger",
    "Brave.Search.QueriesBeforeChurn",
    "Brave.Today.IsEnabled",
    "Brave.Today.UsageDaily",
    "Brave.Today.UsageMonthly",
    "Brave.VerticalTabs.GroupTabs",
    "Brave.VerticalTabs.OpenTabs",
    "Brave.VerticalTabs.PinnedTabs",
    "Brave.Wallet.NewUserBalance",
    "Brave.Wallet.NFTCount",
    "Brave.Wallet.NFTNewUser",
    "Brave.Wallet.OnboardingConversion.3",
    "Brave.Wallet.UsageDaily",
    "Brave.Wallet.UsageMonthly",
    "Brave.Wallet.UsageWeekly"
});

// clang-format on

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_
