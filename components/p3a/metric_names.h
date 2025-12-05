/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_
#define BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "brave/components/p3a/metric_config.h"

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
inline constexpr auto kCollectedTypicalHistograms =
  base::MakeFixedFlatMap<std::string_view, std::optional<MetricConfig>>({
    {"Brave.AIChat.AcquisitionSource", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.AvgPromptCount", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.ChatCount", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.ChatCount.Nebula", MetricConfig{.ephemeral = true,.nebula = true}},
    {"Brave.AIChat.ChatHistoryUsage", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.ContextLimits", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.ContextMenu.FreeUsages", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.ContextMenu.MostUsedAction", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.ContextMenu.PremiumUsages", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.Enabled.2", {}},
    {"Brave.AIChat.FirstChatPrompts", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.FullPageSwitches", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.MaxChatDuration", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.MostUsedContextSource", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.MostUsedEntryPoint", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.NewUserReturning", {}},
    {"Brave.AIChat.OmniboxOpens", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.OmniboxWeekCompare", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.RateLimitStops", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.TabFocus.AvgTabCount", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.TabFocus.Enabled", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.TabFocus.MaxTabCount", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.TabFocus.SessionCount", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.TabFocus.LastUsageTime", MetricConfig{.ephemeral = true,.nebula = true}},
    {"Brave.AIChat.UsageWeekly", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.UsedConversationStarter", {}},
    {"Brave.Ads.ClearData", MetricConfig{.ephemeral = true}},
    {"Brave.Core.BookmarkCount", {}},
    {"Brave.Core.CrashReportsEnabled", {}},
    {"Brave.Core.DomainsLoaded", {}},
    {"Brave.Core.FailedHTTPSUpgrades.2", MetricConfig{.ephemeral = true}},
    {"Brave.Core.FirstPageLoadTime", MetricConfig{.ephemeral = true}},
    {"Brave.Core.IsDefault", MetricConfig{
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kChannel, MetricAttribute::kGeneralPlatform, MetricAttribute::kYoi, MetricAttribute::kSubregion, MetricAttribute::kVersion, MetricAttribute::kWoi},
    }},
    {"Brave.Core.NumberOfExtensions", {}},
    {"Brave.Core.PagesLoaded.NonRewards", {}},
    {"Brave.Core.PagesLoaded.Rewards", {}},
    {"Brave.Core.PagesLoaded.RewardsWallet", {}},
    {"Brave.Core.PagesReloaded", {}},
    {"Brave.Core.TabCount", {}},
    {"Brave.Core.WeeklyUsage", {}},
    {"Brave.Core.WeeklyUsage.Nebula", MetricConfig{.nebula = true}},
    {"Brave.Core.WindowCount.2", {}},
    {"Brave.DNS.AutoSecureRequests.2", MetricConfig{.ephemeral = true}},
    {"Brave.DNS.AutoSecureRequests.Cloudflare.2", MetricConfig{.ephemeral = true}},
    {"Brave.DNS.AutoSecureRequests.Quad9.2", MetricConfig{.ephemeral = true}},
    {"Brave.DNS.AutoSecureRequests.Wikimedia.2", MetricConfig{.ephemeral = true}},
    {"Brave.DNS.SecureSetting", {}},
    {"Brave.Extensions.AdBlock", {}},
    {"Brave.Extensions.SelectManifestV2", {}},
    {"Brave.IOS.IsLikelyDefault", MetricConfig{
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kChannel, MetricAttribute::kYoi, MetricAttribute::kWoi, MetricAttribute::kWeekOfActivation, MetricAttribute::kSubregion, MetricAttribute::kVersion},
      .record_activation_date = true,
    }},
    {"Brave.Importer.ImporterSource.2", {}},
    {"Brave.NTP.CustomizeUsageStatus.2", {}},
    {"Brave.NTP.DefaultPage", {}},
    {"Brave.NTP.SponsoredMediaType", {}},
    {"Brave.Omnibox.SearchCount.NonRewards", {}},
    {"Brave.Omnibox.SearchCount.Rewards", {}},
    {"Brave.Omnibox.SearchCount.RewardsWallet", {}},
    {"Brave.PermissionLifetime.7Days", MetricConfig{.ephemeral = true}},
    {"Brave.P3A.SentAnswersCount", {}},
    {"Brave.Playlist.FirstTimeOffset", MetricConfig{.ephemeral = true}},
    {"Brave.Playlist.NewUserReturning", {}},
    {"Brave.Playlist.UsageDaysInWeek", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.AdTypesEnabled.2", {}},
    {"Brave.Rewards.DesktopPanelCount.2", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.EnabledSource", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.MobileConversion", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.MobilePanelCount.2", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.OfferClicks", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.OffersViewed", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.SearchResultAdsOptin", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.TipsState.2", {}},
    {"Brave.Rewards.ToolbarButtonTrigger", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.WalletBalance.3", {}},
    {"Brave.Rewards.WalletState", {}},
    {"Brave.Search.GoogleWidgetUsage", MetricConfig{.ephemeral = true}},
    {"Brave.Search.Promo.BannerB", {}},
    {"Brave.Search.Promo.BannerC", {}},
    {"Brave.Search.Promo.BannerD", {}},
    {"Brave.Search.Promo.Button", {}},
    {"Brave.Search.Promo.DDGBannerA", {}},
    {"Brave.Search.Promo.DDGBannerB", {}},
    {"Brave.Search.Promo.DDGBannerC", {}},
    {"Brave.Search.Promo.DDGBannerD", {}},
    {"Brave.Search.Promo.NewTabPage", {}},
    {"Brave.Search.QueriesBeforeChurn", MetricConfig{.ephemeral = true}},
    {"Brave.Search.WebDiscoveryAndAds", {}},
    {"Brave.Search.WebDiscoveryDefaultEngine", {}},
    {"Brave.Search.WidgetDefault", {}},
    {"Brave.Search.WidgetUsage", MetricConfig{.ephemeral = true}},
    {"Brave.Shields.AdBlockSetting", {}},
    {"Brave.Shields.DomainAdsSettingsAboveGlobal", {}},
    {"Brave.Shields.DomainAdsSettingsBelowGlobal", {}},
    {"Brave.Shields.FingerprintBlockSetting", {}},
    {"Brave.Shields.UsageStatus", {}},
    {"Brave.Sidebar.Enabled", {}},
    {"Brave.Sync.JoinType", MetricConfig{.ephemeral = true}},
    {"Brave.Sync.Status.2", {}},
    {"Brave.Today.ClickCardDepth", MetricConfig{.ephemeral = true}},
    {"Brave.Today.LastUsageTime", {}},
    {"Brave.Today.NewUserReturning", {}},
    {"Brave.Today.NonRewardsAdViews", {}},
    {"Brave.Today.RewardsAdViews", {}},
    {"Brave.Today.SidebarFilterUsages", MetricConfig{.ephemeral = true}},
    {"Brave.Today.WeeklySessionCount", {}},
    {"Brave.Today.WeeklyTotalCardClicks", MetricConfig{.ephemeral = true}},
    {"Brave.Today.WeeklyTotalCardViews", {}},
    {"Brave.Update.Status", {}},
    {"Brave.VPN.ConnectedDuration", MetricConfig{.ephemeral = true}},
    {"Brave.VPN.HideWidget", MetricConfig{.ephemeral = true}},
    {"Brave.VPN.LastUsageTime", MetricConfig{.record_activation_date = true}},
    {"Brave.VPN.NewUserReturning", MetricConfig{
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kDateOfActivation, MetricAttribute::kDateOfInstall, MetricAttribute::kVersion, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kCountryCode},
      .activation_metric_name = "Brave.VPN.LastUsageTime"
    }},
    {"Brave.VPN.WidgetUsage", MetricConfig{.ephemeral = true}},
    {"Brave.VerticalTabs.GroupTabs", MetricConfig{.ephemeral = true}},
    {"Brave.VerticalTabs.OpenTabs", MetricConfig{.ephemeral = true}},
    {"Brave.VerticalTabs.PinnedTabs", MetricConfig{.ephemeral = true}},
    {"Brave.Wallet.ActiveBtcAccounts", {}},
    {"Brave.Wallet.ActiveEthAccounts", {}},
    {"Brave.Wallet.ActiveSolAccounts", {}},
    {"Brave.Wallet.ActiveZecAccounts", {}},
    {"Brave.Wallet.BtcTransactionSent", {}},
    {"Brave.Wallet.EthTransactionSent", {}},
    {"Brave.Wallet.NFTCount", MetricConfig{.ephemeral = true}},
    {"Brave.Wallet.NewUserReturning", {}},
    {"Brave.Wallet.OnboardingConversion.3", MetricConfig{.ephemeral = true}},
    {"Brave.Wallet.SolTransactionSent", {}},
    {"Brave.Wallet.ZecTransactionSent", {}},
    {"Brave.WebTorrent.UsageWeekly", MetricConfig{.ephemeral = true}},
});

inline constexpr auto kCollectedSlowHistograms =
  base::MakeFixedFlatMap<std::string_view, std::optional<MetricConfig>>({
    {"Brave.AIChat.ContextMenu.LastUsageTime", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.LastUsageTime", MetricConfig{.ephemeral = true}},
    {"Brave.AIChat.UsageMonthly", MetricConfig{.ephemeral = true}},
    {"Brave.Core.PrimaryLang", MetricConfig{}},
    {"Brave.Core.ProfileCount", {}},
    {"Brave.Core.UsageMonthly", {}},
    {"Brave.Extensions.ManifestV2", {}},
    {"Brave.P3A.TestSlowMetric", {}},
    {"Brave.Playlist.LastUsageTime", {}},
    {"Brave.Rewards.PageViewCount", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.RecurringTip", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.TipsSent.2", MetricConfig{.ephemeral = true}},
    {"Brave.Search.SearchSuggest", {}},
    {"Brave.Shields.ForgetFirstParty", {}},
    {"Brave.Speedreader.EnabledSites", {}},
    {"Brave.Speedreader.PageViews", MetricConfig{.ephemeral = true}},
    {"Brave.SplitView.UsageMonthly", {}},
    {"Brave.Sync.EnabledTypes", {}},
    {"Brave.Sync.SyncedObjectsCount.2", {}},
    {"Brave.Today.ChannelCount.2", MetricConfig{.ephemeral = true}},
    {"Brave.Today.DirectFeedsTotal.3", MetricConfig{.ephemeral = true}},
    {"Brave.Today.PublisherCount.2", MetricConfig{.ephemeral = true}},
    {"Brave.Today.UsageMonthly", MetricConfig{.ephemeral = true}},
    {"Brave.Wallet.UsageMonthly", MetricConfig{.ephemeral = true}},
});

inline constexpr auto kCollectedExpressHistograms =
  base::MakeFixedFlatMap<std::string_view, std::optional<MetricConfig>>({
    {"Brave.Ads.SurveyPanelistEnabled", {}},
    {"Brave.AIChat.UsageDaily.2", MetricConfig{
      .ephemeral = true,
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kDateOfActivation, MetricAttribute::kVersion, MetricAttribute::kYoi, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kCountryCode, MetricAttribute::kWoi},
      .record_activation_date = true,
    }},
    {"Brave.Core.IsDefaultDaily", MetricConfig{
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kChannel, MetricAttribute::kDateOfInstall, MetricAttribute::kDateOfActivation, MetricAttribute::kGeneralPlatform, MetricAttribute::kSubregion, MetricAttribute::kVersion},
      .record_activation_date = true,
    }},
    {"Brave.Core.UsageDaily", {}},
    {"Brave.DayZero.Variant", MetricConfig{
      .ephemeral = true,
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kDateOfInstall, MetricAttribute::kVersion, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kCountryCode, MetricAttribute::kRef}
    }},
    {"Brave.NTP.NewTabsCreatedDaily", MetricConfig{.ephemeral = true}},
    {"Brave.PermissionLifetime.24Hours", MetricConfig{.ephemeral = true}},
    {"Brave.Rewards.EnabledInstallationTime", MetricConfig{.ephemeral = true}},
    {"Brave.Search.BackupResultsFailures", MetricConfig{
      .ephemeral = true,
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kVersion, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kCountryCode},
    }},
    {"Brave.Search.BraveDaily", MetricConfig{.ephemeral = true}},
    {"Brave.Search.DefaultEngine.4", MetricConfig{
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kDateOfInstall, MetricAttribute::kVersion, MetricAttribute::kLocaleCountryCode},
    }},
    {"Brave.Search.SwitchEngine.2", MetricConfig{
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kDateOfInstall, MetricAttribute::kVersion, MetricAttribute::kLocaleCountryCode},
    }},
    {"Brave.Search.WebDiscoveryEnabled", {}},
    {"Brave.Today.EnabledSetting", MetricConfig{.attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kDateOfActivation, MetricAttribute::kDateOfInstall, MetricAttribute::kVersion, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kCountryCode}}},
    {"Brave.Today.IsEnabled", MetricConfig{
      .ephemeral = true,
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kDateOfActivation, MetricAttribute::kDateOfInstall, MetricAttribute::kVersion, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kCountryCode},
      .activation_metric_name = "Brave.Today.UsageDaily"
    }},
    {"Brave.Today.UsageDaily", MetricConfig{
      .ephemeral = true,
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kDateOfActivation, MetricAttribute::kVersion, MetricAttribute::kYoi, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kCountryCode, MetricAttribute::kWoi},
      .record_activation_date = true
    }},
    {"Brave.Uptime.BrowserOpenTime.2", MetricConfig{.ephemeral = true}},
    {"Brave.Welcome.InteractionStatus.2", MetricConfig{
      .attributes = MetricAttributes{MetricAttribute::kAnswerIndex, MetricAttribute::kChannel, MetricAttribute::kPlatform, MetricAttribute::kDateOfInstall, MetricAttribute::kSubregion, MetricAttribute::kVersion},
    }},
    {"creativeInstanceId.total.count", {}},
});

}  // namespace p3a

// clang-format on

#endif  // BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_
