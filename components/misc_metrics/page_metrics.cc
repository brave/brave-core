/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/page_metrics.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/misc_metrics/pref_names.h"
#include "brave/components/p3a_utils/bucket.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/browsing_data/core/counters/bookmark_counter.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/core/https_only_mode_metrics.h"
#include "components/security_interstitials/core/metrics_helper.h"

namespace misc_metrics {

namespace {

constexpr const int kPagesLoadedBuckets[] = {0, 10, 50, 100, 500, 1000};
constexpr const int kDomainsLoadedBuckets[] = {0, 4, 10, 30, 50, 100};
constexpr const int kFailedHTTPSUpgradeBuckets[] = {0, 25, 50, 100, 300, 700};
constexpr const int kBookmarkCountBuckets[] = {0,   5,    20,   100,
                                               500, 1000, 5000, 10000};
constexpr const int kFirstPageLoadTimeBuckets[] = {5, 10, 60, 240, 1440};

constexpr base::TimeDelta kReportInterval = base::Minutes(30);
constexpr base::TimeDelta kInitReportDelay = base::Seconds(30);

constexpr char kInterstitialDecisionHistogramName[] =
    "interstitial.https_first_mode.decision";

constexpr size_t kMinDenominatorForFailedHTTPReport = 100;

}  // namespace

using HttpsEvent = security_interstitials::https_only_mode::Event;

PageMetrics::PageMetrics(PrefService* local_state,
                         HostContentSettingsMap* host_content_settings_map,
                         history::HistoryService* history_service,
                         bookmarks::BookmarkModel* bookmark_model,
                         FirstRunTimeCallback first_run_time_callback)
    : local_state_(local_state),
      host_content_settings_map_(host_content_settings_map),
      history_service_(history_service),
      first_run_time_callback_(first_run_time_callback) {
  DCHECK(local_state);
  DCHECK(history_service);

  init_timer_.Start(FROM_HERE, kInitReportDelay, this,
                    &PageMetrics::ReportAllMetrics);

  https_navigation_event_observer_ =
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          security_interstitials::https_only_mode::kEventHistogram,
          base::BindRepeating(&PageMetrics::OnHttpsNavigationEvent,
                              weak_ptr_factory_.GetWeakPtr()));
  interstitial_decision_observer_ =
      std::make_unique<base::StatisticsRecorder::ScopedHistogramSampleObserver>(
          kInterstitialDecisionHistogramName,
          base::BindRepeating(&PageMetrics::OnInterstitialDecisionEvent,
                              weak_ptr_factory_.GetWeakPtr()));
  if (!local_state_->HasPrefPath(
          kMiscMetricsFailedHTTPSUpgradeMetricAddedTime)) {
    local_state_->SetTime(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime,
                          base::Time::Now().LocalMidnight());
  }

  if (bookmark_model) {
    bookmark_counter_ =
        std::make_unique<browsing_data::BookmarkCounter>(bookmark_model);
    bookmark_counter_->InitWithoutPref(
        {}, base::BindRepeating(&PageMetrics::OnBookmarkCountResult,
                                base::Unretained(this)));
  }
}

PageMetrics::~PageMetrics() = default;

void PageMetrics::OnHttpsNavigationEvent(const char* histogram_name,
                                         uint64_t name_hash,
                                         base::HistogramBase::Sample sample) {
  HttpsEvent event = static_cast<HttpsEvent>(sample);
  if (event != HttpsEvent::kUpgradeFailed &&
      event != HttpsEvent::kUpgradeNetError) {
    return;
  }
  InitStorage();
  if (event == HttpsEvent::kUpgradeFailed) {
    VLOG(2) << "PageMetrics: record failed https upgrade";
    failed_https_upgrades_storage_->AddDelta(1);
  } else {
    // If the upgrade failed due to a network error,
    // don't consider it in our metrics. kUpgradeFailed will be reported by
    // at the same time, so we need to subtract to compensate.
    // This will only happen if both the HTTPS and HTTP site variants
    // are unavailable.
    VLOG(2) << "PageMetrics: cancel record failed https upgrade";
    failed_https_upgrades_storage_->SubDelta(1);
  }
}

void PageMetrics::OnInterstitialDecisionEvent(
    const char* histogram_name,
    uint64_t name_hash,
    base::HistogramBase::Sample sample) {
  if (sample != security_interstitials::MetricsHelper::Decision::PROCEED) {
    return;
  }
  InitStorage();
  interstitial_allow_decisions_storage_->AddDelta(1);
}

void PageMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(kMiscMetricsPagesLoadedCount);
  registry->RegisterListPref(kMiscMetricsPagesReloadedCount);
  registry->RegisterListPref(kMiscMetricsInterstitialAllowDecisionCount);
  registry->RegisterListPref(kMiscMetricsFailedHTTPSUpgradeCount);
  registry->RegisterTimePref(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime, {});
}

void PageMetrics::IncrementPagesLoadedCount(bool is_reload) {
  VLOG(2) << "PageMetricsService: increment page load count, is_reload "
          << is_reload;
  InitStorage();
  if (is_reload) {
    pages_reloaded_storage_->AddDelta(1);
  } else {
    if (first_run_time_.is_null()) {
      first_run_time_ = first_run_time_callback_.Run();
    }
    if (first_run_time_ + base::Days(7) > base::Time::Now() &&
        pages_loaded_storage_->GetWeeklySum() == 0) {
      ReportFirstPageLoadTime();
    }
    pages_loaded_storage_->AddDelta(1);
  }
}

void PageMetrics::InitStorage() {
  if (pages_reloaded_storage_ == nullptr) {
    pages_reloaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsPagesReloadedCount);
  }
  if (pages_loaded_storage_ == nullptr) {
    pages_loaded_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsPagesLoadedCount);
  }
  if (interstitial_allow_decisions_storage_ == nullptr) {
    interstitial_allow_decisions_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsInterstitialAllowDecisionCount);
  }
  if (failed_https_upgrades_storage_ == nullptr) {
    failed_https_upgrades_storage_ = std::make_unique<WeeklyStorage>(
        local_state_, kMiscMetricsFailedHTTPSUpgradeCount);
  }
}

void PageMetrics::ReportAllMetrics() {
  InitStorage();
  ReportDomainsLoaded();
  ReportPagesLoaded();
  ReportFailedHTTPSUpgrades();
  ReportBookmarkCount();
  periodic_report_timer_.Start(
      FROM_HERE, base::Time::Now() + kReportInterval,
      base::BindOnce(&PageMetrics::ReportAllMetrics, base::Unretained(this)));
}

void PageMetrics::ReportDomainsLoaded() {
  // Derived from current profile history.
  // Mutiple profiles will result in metric overwrites which is okay.
  history_service_->GetDomainDiversity(
      base::Time::Now(), /*number_of_days_to_report*/ 1,
      history::DomainMetricType::kEnableLast7DayMetric,
      base::BindOnce(&PageMetrics::OnDomainDiversityResult,
                     base::Unretained(this)),
      &history_service_task_tracker_);
}

void PageMetrics::ReportPagesLoaded() {
  // Stores a global count in local state to
  // capture page loads across all profiles.
  uint64_t pages_loaded_count = pages_loaded_storage_->GetWeeklySum();
  uint64_t pages_reloaded_count = pages_reloaded_storage_->GetWeeklySum();
  p3a_utils::RecordToHistogramBucket(kPagesLoadedHistogramName,
                                     kPagesLoadedBuckets, pages_loaded_count);
  p3a_utils::RecordToHistogramBucket(kPagesReloadedHistogramName,
                                     kPagesLoadedBuckets, pages_reloaded_count);
  VLOG(2) << "PageMetricsService: pages loaded report, loaded count = "
          << pages_loaded_count << " reloaded count = " << pages_reloaded_count;
}

void PageMetrics::ReportFailedHTTPSUpgrades() {
  brave_shields::ControlType https_upgrade_settings =
      brave_shields::GetHttpsUpgradeControlType(host_content_settings_map_,
                                                GURL());
  if (https_upgrade_settings == brave_shields::ControlType::ALLOW) {
    if (local_state_->HasPrefPath(
            kMiscMetricsFailedHTTPSUpgradeMetricAddedTime)) {
      // If the metric was recorded in the past, but HTTPS-First or HTTPS-Only
      // mode is not enabled, clear all prefs to save on storage.
      interstitial_allow_decisions_storage_ = nullptr;
      failed_https_upgrades_storage_ = nullptr;
      local_state_->ClearPref(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime);
      local_state_->ClearPref(kMiscMetricsFailedHTTPSUpgradeCount);
      local_state_->ClearPref(kMiscMetricsInterstitialAllowDecisionCount);
      InitStorage();
    }
    // Don't report metric if HTTPS-First or HTTPS-Only mode is not enabled.
    return;
  }

  if (!local_state_->HasPrefPath(
          kMiscMetricsFailedHTTPSUpgradeMetricAddedTime)) {
    local_state_->SetTime(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime,
                          base::Time::Now().LocalMidnight());
  }

  base::Time metric_added_time =
      local_state_->GetTime(kMiscMetricsFailedHTTPSUpgradeMetricAddedTime);

  uint64_t pages_loaded;
  if ((base::Time::Now() - metric_added_time) < base::Days(7)) {
    base::Time now = base::Time::Now().LocalMidnight();
    pages_loaded =
        pages_loaded_storage_->GetPeriodSumInTimeRange(metric_added_time, now) +
        pages_reloaded_storage_->GetPeriodSumInTimeRange(metric_added_time,
                                                         now);
  } else {
    pages_loaded = pages_loaded_storage_->GetWeeklySum() +
                   pages_reloaded_storage_->GetWeeklySum();
  }

  uint64_t interstitial_allow_decisions =
      interstitial_allow_decisions_storage_->GetWeeklySum();
  uint64_t failed_https_upgrades =
      failed_https_upgrades_storage_->GetWeeklySum();

  // We want to exclude interstitial allow decisions (applicable to strict mode)
  // from the denominator. When the user clicks "continue", this triggers a page
  // reload. We want to subtract these types of reloads from the denominator, so
  // we can prevent skewing.
  uint64_t denominator = interstitial_allow_decisions <= pages_loaded
                             ? pages_loaded - interstitial_allow_decisions
                             : 0;

  if (denominator < kMinDenominatorForFailedHTTPReport) {
    VLOG(2) << "PageMetrics: too low for failed https report, denominator = "
            << denominator;
    return;
  }

  double percentage =
      static_cast<double>(failed_https_upgrades) / denominator * 100;
  VLOG(2) << "PageMetrics: failed https upgrade report, failed upgrades = "
          << failed_https_upgrades << ", denominator = " << denominator
          << ", percentage = " << percentage;
  p3a_utils::RecordToHistogramBucket(kFailedHTTPSUpgradesHistogramName,
                                     kFailedHTTPSUpgradeBuckets,
                                     percentage * 100);
}

void PageMetrics::ReportFirstPageLoadTime() {
  auto minutes_since_load = (base::Time::Now() - first_run_time_).InMinutes();
  p3a_utils::RecordToHistogramBucket(kFirstPageLoadTimeHistogramName,
                                     kFirstPageLoadTimeBuckets,
                                     minutes_since_load);
}

void PageMetrics::OnDomainDiversityResult(
    std::pair<history::DomainDiversityResults, history::DomainDiversityResults>
        metrics) {
  if (metrics.first.empty() || metrics.second.empty()) {
    return;
  }
  // The second entry in the pair counts both local, and foreign (synced)
  // visits.
  const history::DomainMetricSet& metric_set = metrics.first.front();
  if (!metric_set.seven_day_metric.has_value()) {
    return;
  }
  int count = metric_set.seven_day_metric->count;
  p3a_utils::RecordToHistogramBucket(kDomainsLoadedHistogramName,
                                     kDomainsLoadedBuckets, count);
  VLOG(2) << "PageMetrics: domains loaded report, count = " << count;
}

void PageMetrics::OnBookmarkCountResult(
    std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
  if (!result || !result->Finished()) {
    return;
  }
  auto* finished_result =
      static_cast<browsing_data::BrowsingDataCounter::FinishedResult*>(
          result.get());
  p3a_utils::RecordToHistogramBucket(kBookmarkCountHistogramName,
                                     kBookmarkCountBuckets,
                                     finished_result->Value());
}

void PageMetrics::ReportBookmarkCount() {
  if (!bookmark_counter_) {
    return;
  }
  bookmark_counter_->Restart();
}

void PageMetrics::OnBraveQuery() {
  UMA_HISTOGRAM_BOOLEAN(kSearchBraveDailyHistogramName, true);
}

}  // namespace misc_metrics
