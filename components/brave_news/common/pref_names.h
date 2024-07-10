// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_

class PrefService;
class PrefRegistrySimple;

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
inline constexpr char kShouldShowToolbarButton[] =
    "brave.today.should_show_toolbar_button";
inline constexpr char kBraveNewsOpenArticlesInNewTab[] =
    "brave.news.open-articles-in-new-tab";

// Dictionary value keys
inline constexpr char kBraveNewsDirectFeedsKeyTitle[] = "title";
inline constexpr char kBraveNewsDirectFeedsKeySource[] = "source";

void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace prefs

bool IsEnabled(PrefService* prefs);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_PREF_NAMES_H_
