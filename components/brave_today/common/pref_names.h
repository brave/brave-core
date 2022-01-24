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
constexpr char kBraveTodayIntroDismissed[] = "brave.today.intro_dismissed";
constexpr char kBraveTodayOptedIn[] = "brave.today.opted_in";
constexpr char kBraveTodayWeeklySessionCount[] =
    "brave.today.p3a_weekly_session_count";
constexpr char kBraveTodayWeeklyCardViewsCount[] =
    "brave.today.p3a_weekly_card_views_count";
constexpr char kBraveTodayWeeklyCardVisitsCount[] =
    "brave.today.p3a_weekly_card_visits_count";
constexpr char kBraveTodayWeeklyDisplayAdViewedCount[] =
    "brave.today.p3a_weekly_display_ad_viewed_count";

}  // namespace prefs

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_COMMON_PREF_NAMES_H_
