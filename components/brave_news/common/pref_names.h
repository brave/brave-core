// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_

namespace brave_news {
namespace prefs {

inline constexpr char kNewTabPageShowToday[] =
    "brave.new_tab_page.show_brave_news";
inline constexpr char kBraveNewsSources[] = "brave.today.sources";
inline constexpr char kBraveNewsChannels[] = "brave.news.channels";
inline constexpr char kBraveNewsDirectFeeds[] = "brave.today.userfeeds";
inline constexpr char kBraveNewsIntroDismissed[] =
    "brave.today.intro_dismissed";
inline constexpr char kBraveNewsOptedIn[] = "brave.today.opted_in";
inline constexpr char kBraveNewsDaysInMonthUsedCount[] =
    "brave.today.p3a_days_in_month_count";
inline constexpr char kShouldShowToolbarButton[] =
    "brave.today.should_show_toolbar_button";
inline constexpr char kBraveNewsWeeklySessionCount[] =
    "brave.today.p3a_weekly_session_count";
inline constexpr char kBraveNewsWeeklyCardViewsCount[] =
    "brave.today.p3a_weekly_card_views_count";
inline constexpr char kBraveNewsWeeklyCardVisitsCount[] =
    "brave.today.p3a_weekly_card_visits_count";
inline constexpr char kBraveNewsWeeklyDisplayAdViewedCount[] =
    "brave.today.p3a_weekly_display_ad_viewed_count";
inline constexpr char kBraveNewsWeeklyAddedDirectFeedsCount[] =
    "brave.today.p3a_weekly_added_direct_feeds_count";
inline constexpr char kBraveNewsTotalCardViews[] =
    "brave.today.p3a_total_card_views";
inline constexpr char kBraveNewsCurrSessionCardViews[] =
    "brave.today.p3a_curr_session_card_views";
inline constexpr char kBraveNewsFirstSessionTime[] =
    "brave.today.p3a_first_session_time";
inline constexpr char kBraveNewsUsedSecondDay[] =
    "brave.today.p3a_used_second_day";
inline constexpr char kBraveNewsLastSessionTime[] =
    "brave.today.p3a_last_session_time";
inline constexpr char kBraveNewsWasEverEnabled[] =
    "brave.today.p3a_was_ever_enabled";
inline constexpr char kBraveNewsOpenArticlesInNewTab[] =
    "brave.news.open-articles-in-new-tab";

// Dictionary value keys
inline constexpr char kBraveNewsDirectFeedsKeyTitle[] = "title";
inline constexpr char kBraveNewsDirectFeedsKeySource[] = "source";

}  // namespace prefs

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_
