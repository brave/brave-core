// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_P3A_H_

#include <cstdint>

class PrefRegistrySimple;
class PrefService;

namespace brave_news {
namespace p3a {

constexpr char kDaysInMonthUsedCountHistogramName[] =
    "Brave.Today.DaysInMonthUsedCount";
constexpr char kWeeklySessionCountHistogramName[] =
    "Brave.Today.WeeklySessionCount";
constexpr char kWeeklyMaxCardVisitsHistogramName[] =
    "Brave.Today.WeeklyMaxCardVisitsCount";
constexpr char kWeeklyMaxCardViewsHistogramName[] =
    "Brave.Today.WeeklyMaxCardViewsCount";
constexpr char kTotalCardViewsHistogramName[] =
    "Brave.Today.WeeklyTotalCardViews";
constexpr char kWeeklyDisplayAdsViewedHistogramName[] =
    "Brave.Today.WeeklyDisplayAdsViewedCount";
constexpr char kDirectFeedsTotalHistogramName[] =
    "Brave.Today.DirectFeedsTotal";
constexpr char kWeeklyAddedDirectFeedsHistogramName[] =
    "Brave.Today.WeeklyAddedDirectFeedsCount";
constexpr char kLastUsageTimeHistogramName[] = "Brave.Today.LastUsageTime";
constexpr char kNewUserReturningHistogramName[] =
    "Brave.Today.NewUserReturning";

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
void RegisterProfilePrefs(PrefRegistrySimple* registry);

}  // namespace p3a
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_P3A_H_
