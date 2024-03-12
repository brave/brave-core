/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_MISC_METRICS_PREF_NAMES_H_

namespace misc_metrics {
inline constexpr char kMiscMetricsBrowserUsageList[] =
    "brave.misc_metrics.browser_usage";
inline constexpr char kMiscMetricsMenuDismissStorage[] =
    "brave.misc_metrics.menu_dismiss_storage";
inline constexpr char kMiscMetricsMenuGroupActionCounts[] =
    "brave.misc_metrics.menu_group_actions";
inline constexpr char kMiscMetricsMenuShownStorage[] =
    "brave.misc_metrics.menu_shown_storage";
inline constexpr char kMiscMetricsPagesLoadedCount[] =
    "brave.core_metrics.pages_loaded";
inline constexpr char kMiscMetricsPagesReloadedCount[] =
    "brave.core_metrics.pages_reloaded";
inline constexpr char kMiscMetricsHTTPAllowedLoadCount[] =
    "brave.misc_metrics.http_allowed_pages_loaded";
inline constexpr char kMiscMetricsFailedHTTPSUpgradeCount[] =
    "brave.misc_metrics.failed_https_upgrades";
inline constexpr char kMiscMetricsFailedHTTPSUpgradeMetricAddedTime[] =
    "brave.misc_metrics.failed_https_upgrade_metric_added_time";
inline constexpr char kMiscMetricsPrivacyHubViews[] =
    "brave.misc_metrics.privacy_hub_views";
inline constexpr char kMiscMetricsOpenTabsStorage[] =
    "brave.misc_metrics.open_tabs_storage";
inline constexpr char kMiscMetricsGroupTabsStorage[] =
    "brave.misc_metrics.group_tabs_storage";
inline constexpr char kMiscMetricsPinnedTabsStorage[] =
    "brave.misc_metrics.pinned_tabs_storage";

inline constexpr char kMiscMetricsSearchSwitchedAwayFromBrave[] =
    "brave.misc_metrics.search_switched_from_brave";
inline constexpr char kMiscMetricsSearchBraveQueryCount[] =
    "brave.misc_metrics.search_brave_query_count";

inline constexpr char kMiscMetricsTotalDnsRequestStorage[] =
    "brave.misc_metrics.total_dns_requests";
inline constexpr char kMiscMetricsUpgradedDnsRequestStorage[] =
    "brave.misc_metrics.upgraded_dns_requests";

inline constexpr char kDailyUptimesListPrefName[] =
    "daily_uptimes";  // DEPRECATED
inline constexpr char kDailyUptimeSumPrefName[] =
    "brave.misc_metrics.uptime_sum";
inline constexpr char kDailyUptimeFrameStartTimePrefName[] =
    "brave.misc_metrics.uptime_frame_start_time";

inline constexpr char kMiscMetricsTabSwitcherNewTabsStorage[] =
    "brave.misc_metrics.tab_switcher_new_tabs_storage";
inline constexpr char kMiscMetricsTotalNewTabsStorage[] =
    "brave.misc_metrics.total_new_tabs_storage";
inline constexpr char kMiscMetricsNewTabLocationBarEntriesStorage[] =
    "brave.misc_metrics.new_tab_location_bar_entries_storage";
inline constexpr char kMiscMetricsTotalLocationBarEntriesStorage[] =
    "brave.misc_metrics.total_location_bar_entries_storage";
}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_PREF_NAMES_H_
