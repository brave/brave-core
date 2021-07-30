// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_PREFS_H_

namespace brave_search {
namespace prefs {

constexpr char kDailyAsked[] = "brave.brave_search.daily_asked";
constexpr char kTotalAsked[] = "brave.brave_search.total_asked";
#if defined(OS_ANDROID)
constexpr char kFetchFromNative[] = "brave.brave_search.fetch_se_from_native";
#endif

}  // namespace prefs
}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_PREFS_H_
