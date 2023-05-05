// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_

#include <cstdint>

class PrefRegistrySimple;
class PrefService;

namespace brave_news {
namespace p3a {

extern const char kDaysInMonthUsedCountHistogramName[];
extern const char kWeeklySessionCountHistogramName[];
extern const char kWeeklyMaxCardVisitsHistogramName[];
extern const char kWeeklyMaxCardViewsHistogramName[];
extern const char kTotalCardViewsHistogramName[];
extern const char kWeeklyDisplayAdsViewedHistogramName[];
extern const char kDirectFeedsTotalHistogramName[];
extern const char kWeeklyAddedDirectFeedsHistogramName[];
extern const char kLastUsageTimeHistogramName[];
extern const char kNewUserReturningHistogramName[];
extern const char kIsEnabledHistogramName[];

void RecordAtInit(PrefService* prefs);
void RecordAtSessionStart(PrefService* prefs);

void RecordWeeklyMaxCardVisitsCount(PrefService* prefs,
                                    uint64_t cards_visited_session_total_count);
void RecordWeeklyMaxCardViewsCount(PrefService* prefs,
                                   uint64_t cards_viewed_session_total_count);
void RecordWeeklyDisplayAdsViewedCount(PrefService* prefs, bool is_add);
void RecordWeeklyAddedDirectFeedsCount(PrefService* prefs, int change);
void RecordDirectFeedsTotal(PrefService* prefs);
void RecordTotalCardViews(PrefService* prefs,
                          uint64_t cards_viewed_session_total_count);
void RecordOptInChange(PrefService* prefs);
void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace p3a
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
