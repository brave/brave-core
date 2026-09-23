// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_P3A_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_P3A_PREF_NAMES_H_

class PrefService;
class PrefRegistrySimple;

namespace brave_news::p3a::prefs {

inline constexpr char kBraveNewsWeeklySessionCount[] =
    "brave.today.p3a_weekly_session_count";
inline constexpr char kBraveNewsWeeklyDisplayAdViewedCount[] =
    "brave.today.p3a_weekly_display_ad_viewed_count";
inline constexpr char kBraveNewsWeeklyAddedDirectFeedsCount[] =
    "brave.today.p3a_weekly_added_direct_feeds_count";
inline constexpr char kBraveNewsTotalCardViews[] =
    "brave.today.p3a_total_card_views";
inline constexpr char kBraveNewsTotalCardVisits[] =
    "brave.today.p3a_total_card_visits";
inline constexpr char kBraveNewsVisitDepthSum[] =
    "brave.today.p3a_card_visit_depth_sum";
inline constexpr char kBraveNewsTotalSidebarFilterUsages[] =
    "brave.today.p3a_total_sidebar_filter_usages";
inline constexpr char kBraveNewsFirstSessionTime[] =
    "brave.today.p3a_first_session_time";
inline constexpr char kBraveNewsUsedSecondDay[] =
    "brave.today.p3a_used_second_day";
inline constexpr char kBraveNewsLastSessionTime[] =
    "brave.today.p3a_last_session_time";
inline constexpr char kBraveNewsWasEverEnabled[] =
    "brave.today.p3a_was_ever_enabled";

void RegisterProfileNewsMetricsPrefs(PrefRegistrySimple* registry);
void RegisterProfileNewsMetricsPrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteProfileNewsMetricsPrefs(PrefService* prefs);

}  // namespace brave_news::p3a::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_P3A_PREF_NAMES_H_
