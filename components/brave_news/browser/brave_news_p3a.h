// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"

class PrefRegistrySimple;
class PrefService;

namespace brave_news::p3a {

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
inline constexpr char kTotalCardClicksHistogramName[] =
    "Brave.Today.WeeklyTotalCardClicks";
inline constexpr char kSidebarFilterUsageHistogramName[] =
    "Brave.Today.SidebarFilterUsages";
inline constexpr char kClickCardDepthHistogramName[] =
    "Brave.Today.ClickCardDepth";
inline constexpr char kChannelCountHistogramName[] = "Brave.Today.ChannelCount";
inline constexpr char kPublisherCountHistogramName[] =
    "Brave.Today.PublisherCount";
inline constexpr char kIsEnabledHistogramName[] = "Brave.Today.IsEnabled";
inline constexpr char kUsageMonthlyHistogramName[] = "Brave.Today.UsageMonthly";
inline constexpr char kUsageDailyHistogramName[] = "Brave.Today.UsageDaily";

class NewsMetrics : public BraveNewsPrefManager::PrefObserver {
 public:
  NewsMetrics(PrefService* prefs, BraveNewsPrefManager& pref_manager);
  ~NewsMetrics() override;

  NewsMetrics(const NewsMetrics&) = delete;
  NewsMetrics& operator=(const NewsMetrics&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);
  static void MigrateObsoleteProfilePrefs(PrefService* prefs);

  void RecordAtInit();
  void RecordAtSessionStart();

  void RecordWeeklyDisplayAdsViewedCount(bool is_add);

  void RecordTotalActionCount(ActionType action, uint64_t count_delta);
  void RecordVisitCardDepth(uint32_t new_visit_card_depth);

  // BraveNewsPrefManager::PrefObserver:
  void OnConfigChanged() override;
  void OnPublishersChanged() override;
  void OnChannelsChanged() override;

 private:
  // These are managed internally, by observing prefs.
  void RecordFeatureEnabledChange();
  void RecordTotalSubscribedCount(SubscribeType subscribe_type,
                                  std::optional<size_t> total);
  void RecordDirectFeedsTotal();
  void RecordWeeklyAddedDirectFeedsCount(int change);

  uint64_t AddToWeeklyStorageAndGetSum(const char* pref_name, int change);
  bool IsMonthlyActiveUser();
  void RecordLastUsageTime();
  void RecordNewUserReturning();
  void RecordWeeklySessionCount(bool is_add);

  raw_ptr<PrefService> prefs_;
  raw_ref<BraveNewsPrefManager> pref_manager_;
  bool was_enabled_ = false;
  uint32_t direct_feed_count_ = 0;

  base::WallClockTimer report_timer_;
  base::flat_map<SubscribeType, size_t> subscription_counts_;

  base::ScopedObservation<BraveNewsPrefManager,
                          BraveNewsPrefManager::PrefObserver>
      observation_{this};
};

}  // namespace brave_news::p3a

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
