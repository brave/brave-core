// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_news {
namespace p3a {

enum class ActionType { kCardView, kCardVisit, kSidebarFilterUsage };

enum class SubscribeType { kChannels, kPublishers };

inline constexpr char kWeeklySessionCountHistogramName[] =
    "Brave.Today.WeeklySessionCount";
inline constexpr char kTotalCardViewsHistogramName[] =
    "Brave.Today.WeeklyTotalCardViews";
inline constexpr char kWeeklyDisplayAdsViewedHistogramName[] =
    "Brave.Today.WeeklyDisplayAdsViewedCount";
inline constexpr char kDirectFeedsTotalHistogramName[] =
    "Brave.Today.DirectFeedsTotal.2";
inline constexpr char kWeeklyAddedDirectFeedsHistogramName[] =
    "Brave.Today.WeeklyAddedDirectFeedsCount";
inline constexpr char kLastUsageTimeHistogramName[] =
    "Brave.Today.LastUsageTime";
inline constexpr char kNewUserReturningHistogramName[] =
    "Brave.Today.NewUserReturning";
inline constexpr char kTotalCardVisitsHistogramName[] =
    "Brave.Today.WeeklyTotalCardVisits";
inline constexpr char kSidebarFilterUsageHistogramName[] =
    "Brave.Today.SidebarFilterUsages";
inline constexpr char kVisitDepthHistogramName[] = "Brave.Today.VisitDepth";
inline constexpr char kChannelCountHistogramName[] = "Brave.Today.ChannelCount";
inline constexpr char kPublisherCountHistogramName[] =
    "Brave.Today.PublisherCount";
inline constexpr char kIsEnabledHistogramName[] = "Brave.Today.IsEnabled";
inline constexpr char kUsageMonthlyHistogramName[] = "Brave.Today.UsageMonthly";
inline constexpr char kUsageDailyHistogramName[] = "Brave.Today.UsageDaily";

class NewsMetrics {
 public:
  explicit NewsMetrics(PrefService* prefs);
  ~NewsMetrics();

  NewsMetrics(const NewsMetrics&) = delete;
  NewsMetrics& operator=(const NewsMetrics&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);
  static void MigrateObsoleteProfilePrefs(PrefService* prefs);

  void RecordAtInit();
  void RecordAtSessionStart();

  void RecordWeeklyDisplayAdsViewedCount(bool is_add);
  void RecordWeeklyAddedDirectFeedsCount(int change);
  void RecordDirectFeedsTotal();

  void RecordTotalActionCount(ActionType action, uint64_t count_delta);
  void RecordVisitCardDepth(uint32_t new_visit_card_depth);
  void RecordFeatureEnabledChange();

  void RecordTotalSubscribedCount(SubscribeType subscribe_type,
                                  std::optional<size_t> total);

 private:
  bool IsNewsEnabled();
  uint64_t AddToWeeklyStorageAndGetSum(const char* pref_name, int change);
  bool IsMonthlyActiveUser();
  void RecordLastUsageTime();
  void RecordNewUserReturning();
  void RecordWeeklySessionCount(bool is_add);

  raw_ptr<PrefService> prefs_;
  base::WallClockTimer report_timer_;
  base::flat_map<SubscribeType, size_t> subscription_counts_;
};

}  // namespace p3a
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
