// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_PREF_NAMES_H_

namespace brave_news {
namespace prefs {

constexpr char kNewTabPageShowToday[] = "brave.new_tab_page.show_brave_today";
constexpr char kBraveTodaySources[] = "brave.today.sources";
constexpr char kBraveTodayDirectFeeds[] = "brave.today.userfeeds";
constexpr char kBraveTodayIntroDismissed[] = "brave.today.intro_dismissed";
constexpr char kBraveTodayOptedIn[] = "brave.today.opted_in";
constexpr char kBraveTodayDaysInMonthUsedCount[] =
    "brave.today.p3a_days_in_month_count";
constexpr char kBraveTodayWeeklySessionCount[] =
    "brave.today.p3a_weekly_session_count";
constexpr char kBraveTodayWeeklyCardViewsCount[] =
    "brave.today.p3a_weekly_card_views_count";
constexpr char kBraveTodayWeeklyCardVisitsCount[] =
    "brave.today.p3a_weekly_card_visits_count";
constexpr char kBraveTodayWeeklyDisplayAdViewedCount[] =
    "brave.today.p3a_weekly_display_ad_viewed_count";
constexpr char kBraveTodayWeeklyAddedDirectFeedsCount[] =
    "brave.today.p3a_weekly_added_direct_feeds_count";
constexpr char kBraveTodayTotalCardViews[] = "brave.today.p3a_total_card_views";
constexpr char kBraveTodayCurrSessionCardViews[] =
    "brave.today.p3a_curr_session_card_views";
constexpr char kBraveTodayFirstSessionTime[] =
    "brave.today.p3a_first_session_time";
constexpr char kBraveTodayUsedSecondDay[] = "brave.today.p3a_used_second_day";
constexpr char kBraveTodayLastSessionTime[] =
    "brave.today.p3a_last_session_time";

// Dictionary value keys
constexpr char kBraveTodayDirectFeedsKeyTitle[] = "title";
constexpr char kBraveTodayDirectFeedsKeySource[] = "source";

}  // namespace prefs

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_PREF_NAMES_H_
