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

inline constexpr char kWeeklySessionCountHistogramName[] =
    "Brave.Today.WeeklySessionCount";
inline constexpr char kTotalCardViewsHistogramName[] =
    "Brave.Today.WeeklyTotalCardViews";
inline constexpr char kWeeklyDisplayAdsViewedHistogramName[] =
    "Brave.Today.WeeklyDisplayAdsViewedCount";
inline constexpr char kDirectFeedsTotalHistogramName[] =
    "Brave.Today.DirectFeedsTotal";
inline constexpr char kWeeklyAddedDirectFeedsHistogramName[] =
    "Brave.Today.WeeklyAddedDirectFeedsCount";
inline constexpr char kLastUsageTimeHistogramName[] =
    "Brave.Today.LastUsageTime";
inline constexpr char kNewUserReturningHistogramName[] =
    "Brave.Today.NewUserReturning";
inline constexpr char kIsEnabledHistogramName[] = "Brave.Today.IsEnabled";
inline constexpr char kUsageMonthlyHistogramName[] = "Brave.Today.UsageMonthly";
inline constexpr char kUsageDailyHistogramName[] = "Brave.Today.UsageDaily";

void RecordAtInit(PrefService* prefs);
void RecordAtSessionStart(PrefService* prefs);

void RecordWeeklyDisplayAdsViewedCount(PrefService* prefs, bool is_add);
void RecordWeeklyAddedDirectFeedsCount(PrefService* prefs, int change);
void RecordDirectFeedsTotal(PrefService* prefs);

void RecordTotalCardViews(PrefService* prefs, uint64_t count_delta);
void RecordFeatureEnabledChange(PrefService* prefs);

void RegisterProfilePrefs(PrefRegistrySimple* registry);
void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteProfilePrefs(PrefService* prefs);

}  // namespace p3a
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
