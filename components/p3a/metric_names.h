/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_
#define BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_

#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_piece.h"

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
  base::MakeFixedFlatSet<base::StringPiece>({
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
    "Brave.Importer.ImporterSource.2",
    "Brave.NTP.CustomizeUsageStatus",
    "Brave.NTP.NewTabsCreated.2",
    "Brave.NTP.SponsoredImagesEnabled",
    "Brave.NTP.SponsoredNewTabsCreated",
    "Brave.Omnibox.SearchCount",
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
    "Brave.Uptime.BrowserOpenMinutes",
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
    "Brave.P2A.TotalAdOpportunities",
    "Brave.P2A.AdOpportunitiesPerSegment.architecture",
    "Brave.P2A.AdOpportunitiesPerSegment.artsentertainment",
    "Brave.P2A.AdOpportunitiesPerSegment.automotive",
    "Brave.P2A.AdOpportunitiesPerSegment.business",
    "Brave.P2A.AdOpportunitiesPerSegment.careers",
    "Brave.P2A.AdOpportunitiesPerSegment.cellphones",
    "Brave.P2A.AdOpportunitiesPerSegment.crypto",
    "Brave.P2A.AdOpportunitiesPerSegment.education",
    "Brave.P2A.AdOpportunitiesPerSegment.familyparenting",
    "Brave.P2A.AdOpportunitiesPerSegment.fashion",
    "Brave.P2A.AdOpportunitiesPerSegment.folklore",
    "Brave.P2A.AdOpportunitiesPerSegment.fooddrink",
    "Brave.P2A.AdOpportunitiesPerSegment.gaming",
    "Brave.P2A.AdOpportunitiesPerSegment.healthfitness",
    "Brave.P2A.AdOpportunitiesPerSegment.history",
    "Brave.P2A.AdOpportunitiesPerSegment.hobbiesinterests",
    "Brave.P2A.AdOpportunitiesPerSegment.home",
    "Brave.P2A.AdOpportunitiesPerSegment.law",
    "Brave.P2A.AdOpportunitiesPerSegment.military",
    "Brave.P2A.AdOpportunitiesPerSegment.other",
    "Brave.P2A.AdOpportunitiesPerSegment.personalfinance",
    "Brave.P2A.AdOpportunitiesPerSegment.pets",
    "Brave.P2A.AdOpportunitiesPerSegment.realestate",
    "Brave.P2A.AdOpportunitiesPerSegment.science",
    "Brave.P2A.AdOpportunitiesPerSegment.sports",
    "Brave.P2A.AdOpportunitiesPerSegment.technologycomputing",
    "Brave.P2A.AdOpportunitiesPerSegment.travel",
    "Brave.P2A.AdOpportunitiesPerSegment.weather",
    "Brave.P2A.AdOpportunitiesPerSegment.untargeted",
    // Ad Impressions
    "Brave.P2A.TotalAdImpressions",
    "Brave.P2A.AdImpressionsPerSegment.architecture",
    "Brave.P2A.AdImpressionsPerSegment.artsentertainment",
    "Brave.P2A.AdImpressionsPerSegment.automotive",
    "Brave.P2A.AdImpressionsPerSegment.business",
    "Brave.P2A.AdImpressionsPerSegment.careers",
    "Brave.P2A.AdImpressionsPerSegment.cellphones",
    "Brave.P2A.AdImpressionsPerSegment.crypto",
    "Brave.P2A.AdImpressionsPerSegment.education",
    "Brave.P2A.AdImpressionsPerSegment.familyparenting",
    "Brave.P2A.AdImpressionsPerSegment.fashion",
    "Brave.P2A.AdImpressionsPerSegment.folklore",
    "Brave.P2A.AdImpressionsPerSegment.fooddrink",
    "Brave.P2A.AdImpressionsPerSegment.gaming",
    "Brave.P2A.AdImpressionsPerSegment.healthfitness",
    "Brave.P2A.AdImpressionsPerSegment.history",
    "Brave.P2A.AdImpressionsPerSegment.hobbiesinterests",
    "Brave.P2A.AdImpressionsPerSegment.home",
    "Brave.P2A.AdImpressionsPerSegment.law",
    "Brave.P2A.AdImpressionsPerSegment.military",
    "Brave.P2A.AdImpressionsPerSegment.other",
    "Brave.P2A.AdImpressionsPerSegment.personalfinance",
    "Brave.P2A.AdImpressionsPerSegment.pets",
    "Brave.P2A.AdImpressionsPerSegment.realestate",
    "Brave.P2A.AdImpressionsPerSegment.science",
    "Brave.P2A.AdImpressionsPerSegment.sports",
    "Brave.P2A.AdImpressionsPerSegment.technologycomputing",
    "Brave.P2A.AdImpressionsPerSegment.travel",
    "Brave.P2A.AdImpressionsPerSegment.weather",
    "Brave.P2A.AdImpressionsPerSegment.untargeted"
});

constexpr inline auto kCollectedSlowHistograms =
  base::MakeFixedFlatSet<base::StringPiece>({
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
    "Brave.Wallet.UsageMonthly"
});

constexpr inline auto kCollectedExpressHistograms =
  base::MakeFixedFlatSet<base::StringPiece>({
    "Brave.AIChat.UsageDaily",
    "Brave.Core.UsageDaily",
    "Brave.Rewards.EnabledInstallationTime",
    "Brave.Today.IsEnabled",
    "Brave.Today.UsageDaily",
    "Brave.Wallet.UsageDaily"
});

// List of metrics that should only be sent once per latest histogram update.
// Once the metric value has been sent, the value will be removed from the log store.
constexpr inline auto kEphemeralHistograms =
  base::MakeFixedFlatSet<base::StringPiece>({
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
