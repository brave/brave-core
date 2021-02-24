// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/brave_today_service.h"


#include <memory>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/p3a/brave_p3a_service.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/pref_registry/pref_registry_syncable.h"

namespace brave_today {

// static
void BraveTodayService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kBraveTodaySources);
  registry->RegisterBooleanPref(prefs::kBraveTodayIntroDismissed, false);
  registry->RegisterListPref(prefs::kBraveTodayWeeklySessionCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardViewsCount);
  registry->RegisterListPref(prefs::kBraveTodayWeeklyCardVisitsCount);
}

BraveTodayService::BraveTodayService(brave::BraveP3AService* p3a_service,
    brave_ads::AdsService* ads_service,
    PrefService* prefs, PrefService* local_state)
        : p3a_service_(p3a_service),
          ads_service_(ads_service),
          prefs_(prefs) {
  // Ensure we send the value "0" if the feature has not been used in the metric
  // time periods we are concerned with.
  p3a_service_->AddCollector(this);
}

BraveTodayService::~BraveTodayService() {
  p3a_service_->RemoveCollector(this);
}

void BraveTodayService::RecordUserHasInteracted() {
  // Track if user has ever scrolled to Brave Today.
  // TODO(petemill): Save a flag and report 0 if flag not met.
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.HasEverInteracted", 1, 1);
  // Track how many times in the past week
  // user has scrolled to Brave Today.
  WeeklyStorage session_count_storage(prefs_, kBraveTodayWeeklySessionCount);
  session_count_storage.AddDelta(1);
  SendMetricSessionCount();
}

void BraveTodayService::RecordItemVisit(int cards_visited_this_session) {
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(prefs_, prefs::kBraveTodayWeeklyCardVisitsCount);
  storage.ReplaceTodaysValueIfGreater(cards_visited_this_session);
  SendMetricItemVisits();
}

void BraveTodayService::RecordPromotedItemVisit(
    std::string item_id, std::string creative_instance_id) {
    ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::BraveAdsPromotedContentAdEventType::kClicked);
}

void BraveTodayService::RecordItemViews(int cards_viewed_this_session) {
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(prefs_, kBraveTodayWeeklyCardViewsCount);
  storage.ReplaceTodaysValueIfGreater(cards_viewed_this_session);
  SendMetricItemViews();
}

void BraveTodayService::RecordPromotedItemView(
    std::string item_id, std::string creative_instance_id) {
  ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::BraveAdsPromotedContentAdEventType::kViewed);
}

void BraveTodayService::SendMetricSessionCount() {
  WeeklyStorage session_count_storage(prefs_, kBraveTodayWeeklySessionCount);
  uint64_t total_session_count = session_count_storage.GetWeeklySum();
  constexpr int kSessionCountBuckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  const int* it_count =
      std::lower_bound(kSessionCountBuckets, std::end(kSessionCountBuckets),
                      total_session_count);
  int answer = it_count - kSessionCountBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklySessionCount", answer,
                             base::size(kSessionCountBuckets) + 1);
}

void BraveTodayService::SendMetricItemVisits() {
  WeeklyStorage storage(prefs_, prefs::kBraveTodayWeeklyCardVisitsCount);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 3, 6, 10, 15, 100};
  const int* it_count =
      std::lower_bound(kBuckets, std::end(kBuckets),
                      total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardVisitsCount", answer,
                            base::size(kBuckets) + 1);
}

void BraveTodayService::SendMetricItemViews() {
  WeeklyStorage storage(prefs_, kBraveTodayWeeklyCardViewsCount);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 4, 12, 20, 40, 80, 1000};
  const int* it_count =
      std::lower_bound(kBuckets, std::end(kBuckets),
                      total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardViewsCount", answer,
                             base::size(kBuckets) + 1);
}


void BraveTodayService::CollectMetrics() {
  SendMetricSessionCount();
  SendMetricItemVisits();
  SendMetricItemViews();
}

}  // namespace brave_today
