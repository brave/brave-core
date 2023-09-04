// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_

namespace brave_news {
namespace prefs {

constexpr char kNewTabPageShowToday[] = "brave.new_tab_page.show_brave_news";
constexpr char kBraveNewsSources[] = "brave.today.sources";
constexpr char kBraveNewsChannels[] = "brave.news.channels";
constexpr char kBraveNewsDirectFeeds[] = "brave.today.userfeeds";
constexpr char kBraveNewsIntroDismissed[] = "brave.today.intro_dismissed";
constexpr char kBraveNewsOptedIn[] = "brave.today.opted_in";
constexpr char kBraveNewsDaysInMonthUsedCount[] =
    "brave.today.p3a_days_in_month_count";
constexpr char kShouldShowToolbarButton[] =
    "brave.today.should_show_toolbar_button";
constexpr char kBraveNewsWeeklySessionCount[] =
    "brave.today.p3a_weekly_session_count";
constexpr char kBraveNewsWeeklyCardViewsCount[] =
    "brave.today.p3a_weekly_card_views_count";
constexpr char kBraveNewsWeeklyCardVisitsCount[] =
    "brave.today.p3a_weekly_card_visits_count";
constexpr char kBraveNewsWeeklyDisplayAdViewedCount[] =
    "brave.today.p3a_weekly_display_ad_viewed_count";
constexpr char kBraveNewsWeeklyAddedDirectFeedsCount[] =
    "brave.today.p3a_weekly_added_direct_feeds_count";
constexpr char kBraveNewsTotalCardViews[] = "brave.today.p3a_total_card_views";
constexpr char kBraveNewsCurrSessionCardViews[] =
    "brave.today.p3a_curr_session_card_views";
constexpr char kBraveNewsFirstSessionTime[] =
    "brave.today.p3a_first_session_time";
constexpr char kBraveNewsUsedSecondDay[] = "brave.today.p3a_used_second_day";
constexpr char kBraveNewsLastSessionTime[] =
    "brave.today.p3a_last_session_time";
constexpr char kBraveNewsWasEverEnabled[] = "brave.today.p3a_was_ever_enabled";

// Dictionary value keys
constexpr char kBraveNewsDirectFeedsKeyTitle[] = "title";
constexpr char kBraveNewsDirectFeedsKeySource[] = "source";

}  // namespace prefs

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_
