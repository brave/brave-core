// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_P3A_H_

#include <cstdint>

#include "base/memory/raw_ptr.h"
#include "brave/components/p3a_utils/count_report_limiter.h"

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

class NewsP3A {
 public:
  explicit NewsP3A(PrefService* prefs);
  ~NewsP3A();

  void RecordAtInit();
  void RecordAtSessionStart();

  void RecordWeeklyMaxCardVisitsCount(
      uint64_t cards_visited_session_total_count);
  void RecordWeeklyDisplayAdsViewedCount(bool is_add);
  void RecordWeeklyAddedDirectFeedsCount(int change);
  void RecordDirectFeedsTotal();
  void RecordCardViewMetrics(uint64_t cards_viewed_session_total_count);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

 private:
  void OnViewReportInterval(uint64_t new_card_views);
  void RecordWeeklyMaxCardViewsCount(uint64_t new_card_views);
  void RecordTotalCardViews(uint64_t new_card_views);

  raw_ptr<PrefService> prefs_;
  uint64_t raw_session_card_view_count_;
  uint64_t limited_session_card_view_count_;
  p3a_utils::CountReportLimiter count_report_limiter_;
};

}  // namespace p3a
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_BRAVE_NEWS_P3A_H_
